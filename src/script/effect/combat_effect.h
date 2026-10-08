#pragma once

#include "character/character.h"
#include "character/character_class.h"
#include "character/character_container.h"
#include "character/character_game_data.h"
#include "character/character_reference.h"
#include "character/party.h"
#include "character/monster_type.h"
#include "database/data_entry_container.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "domain/domain.h"
#include "game/combat.h"
#include "game/game.h"
#include "item/item_type.h"
#include "item/object_type.h"
#include "item/trap_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/target_variant.h"
#include "species/species.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/qunique_ptr.h"
#include "util/random.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"

namespace metternich {

class combat_effect final : public effect<const domain>
{
public:

	class object final
	{
	public:
		explicit object(const gsml_data &scope)
		{
			this->object_type = object_type::get(scope.get_tag());

			scope.for_each_element([this](const gsml_property &property) {
				if (property.get_key() == "description") {
					this->description = property.get_value();
				} else if (property.get_key() == "trap") {
					this->trap = trap_type::get(property.get_value());
				} else if (property.get_key() == "placement") {
					this->placement = magic_enum::enum_cast<combat_placement>(property.get_value()).value();
				} else {
					assert_throw(false);
				}
			}, [this](const gsml_data &child_scope) {
				if (child_scope.get_tag() == "placement_offset") {
					this->placement_offset = child_scope.to_point();
				} else if (child_scope.get_tag() == "on_used") {
					this->use_effects = std::make_unique<effect_list<const character>>();
					this->use_effects->process_gsml_data(child_scope);
				} else {
					assert_throw(false);
				}
			});
		}

		const metternich::object_type *get_object_type() const
		{
			return this->object_type;
		}

		const std::string &get_description() const
		{
			return this->description;
		}

		const trap_type *get_trap() const
		{
			return this->trap;
		}

		const combat_placement get_placement() const
		{
			return this->placement;
		}

		const QPoint &get_placement_offset() const
		{
			return this->placement_offset;
		}

		const effect_list<const character> *get_use_effects() const
		{
			return this->use_effects.get();
		}

	private:
		const metternich::object_type *object_type = nullptr;
		std::string description;
		const trap_type *trap = nullptr;
		combat_placement placement = combat_placement::right;
		QPoint placement_offset = QPoint(0, 0);
		std::unique_ptr<effect_list<const character>> use_effects;
	};

	explicit combat_effect(const gsml_operator effect_operator) : effect<const domain>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "combat";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();

		if (key == "attacker") {
			this->attacker = string::to_bool(property.get_value());
		} else {
			effect::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();
		const std::vector<std::string> &values = scope.get_values();

		if (tag == "map_size") {
			this->map_size = scope.to_size();
		} else if (tag == "enemy_characters") {
			for (const std::string &value : values) {
				this->enemy_characters.push_back(string_to_target_variant<const character>(value));
			}
		} else if (tag == "objects") {
			scope.for_each_child([this](const gsml_data &child_scope) {
				auto object = std::make_unique<combat_effect::object>(child_scope);
				++this->object_counts[object->get_object_type()];
				this->objects.push_back(std::move(object));
			});
		} else {
			effect::process_gsml_scope(scope);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(const domain *scope, context &ctx) const override
	{
		assert_throw(ctx.party != nullptr);

		std::vector<std::shared_ptr<character_reference>> generated_characters;
		character_map<const enemy *> character_enemy_infos;
		const std::vector<const character *> enemy_characters = co_await this->get_enemy_characters(ctx, generated_characters, character_enemy_infos);

		auto enemy_party = std::make_unique<party>(enemy_characters);

		auto combat = make_qunique<metternich::combat>(this->attacker ? ctx.party.get() : enemy_party.get(), this->attacker ? enemy_party.get() : ctx.party.get(), this->map_size);

		for (const std::unique_ptr<object> &object : this->objects) {
			combat->add_object(object->get_object_type(), object->get_use_effects(), object->get_trap(), object->get_description(), object->get_placement(), object->get_placement_offset());
		}

		combat->set_generated_characters(generated_characters);
		combat->set_generated_party(std::move(enemy_party));

		combat->set_scope(scope);

		context combat_ctx = ctx;
		combat_ctx.in_combat = true;
		combat->set_context(combat_ctx);

		for (const auto &[character, enemy] : character_enemy_infos) {
			combat_character_info *character_info = combat->get_character_info(character);
			assert_throw(character_info != nullptr);

			character_info->set_placement(enemy->get_placement());
			character_info->set_placement_offset(enemy->get_placement_offset());
			character_info->set_kill_effects(enemy->get_kill_effects());
		}

		combat->initialize();

		if (scope == game::get()->get_player_domain()) {
			game::get()->set_current_combat(std::move(combat));
		} else {
			QTimer::singleShot(0, [combat = std::move(combat)]() -> QCoro::Task<void> {
				co_await combat->start_coro();
			});
		}
	}

	virtual std::string get_assignment_string(const domain *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(prefix);

		assert_throw(ctx.party != nullptr);

		std::string str = "Party:";
		for (const character *party_character : ctx.party->get_characters()) {
			std::string character_class_string;
			const character_class *character_class = party_character->get_game_data()->get_character_class();
			if (character_class != nullptr) {
				character_class_string += std::format(" {} {}", party_character->get_game_data()->get_character_class_name(), party_character->get_game_data()->get_level());
			}
			str += "\n" + std::string(indent + 1, '\t') + std::format("{} ({}{} HP {}/{})", party_character->get_game_data()->get_full_name(), party_character->get_species()->get_name(party_character->get_gender()), character_class_string, party_character->get_game_data()->get_health(), party_character->get_game_data()->get_max_health());
		}

		str += "\n" + std::string(indent, '\t') + "Does combat against:";

		for (const target_variant<const character> &enemy_character : this->enemy_characters) {
			const character *character = this->get_enemy_character(enemy_character, ctx);

			std::string character_class_string;
			const character_class *character_class = character->get_game_data()->get_character_class();
			if (character_class != nullptr) {
				character_class_string += std::format(" {} {}", character->get_game_data()->get_character_class_name(), character->get_game_data()->get_level());
			}
			str += "\n" + std::string(indent + 1, '\t') + std::format("{} ({}{})", character->get_game_data()->get_full_name(), character->get_species()->get_name(character->get_gender()), character_class_string);
		}

		for (const auto &[object_type, quantity] : this->object_counts) {
			str += "\n" + std::string(indent + 1, '\t') + std::to_string(quantity) + "x" + object_type->get_name();
		}

		return str;
	}

	[[nodiscard]] QCoro::Task<std::vector<const character *>> get_enemy_characters(const read_only_context &ctx, std::vector<std::shared_ptr<character_reference>> &generated_characters, character_map<const enemy *> &character_enemy_infos) const
	{
		std::vector<const character *> enemy_characters;

		for (const target_variant<const character> &enemy_character_variant : this->enemy_characters) {
			const character *enemy_character = this->get_enemy_character(enemy_character_variant, ctx);

			//ensure the enemy character's HP and mana are completely recovered
			co_await enemy_character->get_game_data()->fully_recover();

			enemy_characters.push_back(enemy_character);
		}

		co_return vector::shuffled(enemy_characters);
	}

	const character *get_enemy_character(const target_variant<const character> &target_variant, const read_only_context &ctx) const
	{
		if (std::holds_alternative<const character *>(target_variant)) {
			return std::get<const character *>(target_variant);
		} else if (std::holds_alternative<std::string>(target_variant)) {
			const std::string name = std::get<std::string>(target_variant);
			const character *character = ctx.get_saved_scope<const metternich::character>(name);
			return character;
		}

		assert_throw(false);
		return nullptr;
	}

private:
	bool attacker = true;
	QSize map_size;
	std::vector<target_variant<const character>> enemy_characters;
	std::vector<std::unique_ptr<object>> objects;
	data_entry_map<object_type, int> object_counts;
};

}
