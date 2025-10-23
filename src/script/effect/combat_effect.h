#pragma once

#include "character/character.h"
#include "character/character_class.h"
#include "character/character_container.h"
#include "character/character_game_data.h"
#include "character/character_reference.h"
#include "character/party.h"
#include "character/monster_type.h"
#include "database/data_entry_container.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "domain/country_government.h"
#include "domain/domain.h"
#include "engine_interface.h"
#include "game/combat.h"
#include "game/game.h"
#include "infrastructure/dungeon.h"
#include "infrastructure/dungeon_area.h"
#include "item/item_type.h"
#include "item/object_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/target_variant.h"
#include "species/species.h"
#include "ui/portrait.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/qunique_ptr.h"
#include "util/random.h"
#include "util/string_conversion_util.h"
#include "util/vector_random_util.h"

namespace metternich {

class enemy final
{
public:
	explicit enemy(const gsml_data &scope)
	{
		this->monster_type = monster_type::get(scope.get_tag());

		scope.for_each_element([&](const gsml_property &property) {
			if (property.get_key() == "hit_points") {
				this->hit_points = std::stoi(property.get_value());
			} else {
				assert_throw(false);
			}
		}, [&](const gsml_data &child_scope) {
			if (child_scope.get_tag() == "items") {
				for (const std::string &value : child_scope.get_values()) {
					this->items.push_back(item_type::get(value));
				}

				child_scope.for_each_property([&](const gsml_property &property) {
					const std::string &key = property.get_key();
					const std::string &value = property.get_value();
					const item_type *item_type = item_type::get(key);
					const int quantity = std::stoi(value);

					for (int i = 0; i < quantity; ++i) {
						this->items.push_back(item_type);
					}
				});
			} else if (child_scope.get_tag() == "on_killed") {
				this->kill_effects = std::make_unique<effect_list<const domain>>();
				this->kill_effects->process_gsml_data(child_scope);
			} else {
				assert_throw(false);
			}
		});
	}

	const metternich::monster_type *get_monster_type() const
	{
		return this->monster_type;
	}

	int get_hit_points()
	{
		return this->hit_points;
	}

	const std::vector<const item_type *> &get_items() const
	{
		return this->items;
	}

	const effect_list<const domain> *get_kill_effects() const
	{
		return this->kill_effects.get();
	}

private:
	const metternich::monster_type *monster_type = nullptr;
	int hit_points = 0;
	std::vector<const item_type *> items;
	std::unique_ptr<effect_list<const domain>> kill_effects;
};

class combat_effect final : public effect<const domain>
{
public:

	class object final
	{
	public:
		explicit object(const gsml_data &scope)
		{
			this->object_type = object_type::get(scope.get_tag());

			scope.for_each_element([&](const gsml_property &property) {
				Q_UNUSED(property);
				assert_throw(false);
			}, [&](const gsml_data &child_scope) {
				if (child_scope.get_tag() == "on_used") {
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

		const effect_list<const character> *get_use_effects() const
		{
			return this->use_effects.get();
		}

	private:
		const metternich::object_type *object_type = nullptr;
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
		} else if (key == "surprise") {
			this->surprise = string::to_bool(property.get_value());
		} else if (key == "to_hit_modifier") {
			this->to_hit_modifier = std::stoi(property.get_value());
		} else if (key == "retreat_allowed") {
			this->retreat_allowed = string::to_bool(property.get_value());
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
		} else if (tag == "enemies") {
			scope.for_each_element([&](const gsml_property &property) {
				const std::string &key = property.get_key();
				const monster_type *monster_type = monster_type::get(key);

				const std::string &value = property.get_value();
				if (string::is_number(value)) {
					this->enemy_counts[monster_type] = std::stoi(value);
				} else {
					this->enemy_counts[monster_type] = dice(value);
				}
			}, [&](const gsml_data &child_scope) {
				auto enemy = std::make_unique<metternich::enemy>(child_scope);
				if (!this->enemy_counts.contains(enemy->get_monster_type())) {
					this->enemy_counts[enemy->get_monster_type()] = 0;
				}
				this->enemies.push_back(std::move(enemy));
			});
		} else if (tag == "enemy_characters") {
			for (const std::string &value : values) {
				this->enemy_characters.push_back(string_to_target_variant<const character>(value));
			}
		} else if (tag == "objects") {
			scope.for_each_child([&](const gsml_data &child_scope) {
				auto object = std::make_unique<combat_effect::object>(child_scope);
				++this->object_counts[object->get_object_type()];
				this->objects.push_back(std::move(object));
			});
		} else if (tag == "on_victory") {
			this->victory_effects = std::make_unique<effect_list<const domain>>();
			this->victory_effects->process_gsml_data(scope);
		} else if (tag == "on_defeat") {
			this->defeat_effects = std::make_unique<effect_list<const domain>>();
			this->defeat_effects->process_gsml_data(scope);
		} else {
			effect::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(const domain *scope, context &ctx) const override
	{
		assert_throw(ctx.party != nullptr);

		std::vector<std::shared_ptr<character_reference>> generated_characters;
		character_map<const effect_list<const domain> *> character_kill_effects;
		const std::vector<const character *> enemy_characters = this->get_enemy_characters(ctx, generated_characters, character_kill_effects);

		auto enemy_party = std::make_unique<party>(enemy_characters);

		auto combat = make_qunique<metternich::combat>(this->attacker ? ctx.party.get() : enemy_party.get(), this->attacker ? enemy_party.get() : ctx.party.get(), this->map_size);

		for (const std::unique_ptr<object> &object : this->objects) {
			combat->add_object(object->get_object_type(), object->get_use_effects());
		}

		if (ctx.dungeon_area != nullptr && ctx.dungeon_area->get_terrain() != nullptr) {
			combat->set_base_terrain(ctx.dungeon_area->get_terrain());
		} else if (ctx.dungeon_site != nullptr && ctx.dungeon_site->get_game_data()->get_dungeon()->get_terrain() != nullptr) {
			combat->set_base_terrain(ctx.dungeon_site->get_game_data()->get_dungeon()->get_terrain());
		}

		combat->set_surprise(this->surprise);
		combat->set_attacker_to_hit_modifier(this->attacker ? this->to_hit_modifier : 0);
		combat->set_defender_to_hit_modifier(this->attacker ? 0 : this->to_hit_modifier);
		combat->set_attacker_retreat_allowed(this->attacker ? this->retreat_allowed : false);
		combat->set_defender_retreat_allowed(this->attacker ? false : this->retreat_allowed);

		combat->set_generated_characters(generated_characters);
		combat->set_generated_party(std::move(enemy_party));

		combat->set_scope(scope);

		context combat_ctx = ctx;
		combat_ctx.in_combat = true;
		combat->set_context(combat_ctx);
		combat->set_character_kill_effects(character_kill_effects);
		combat->set_victory_effects(this->victory_effects.get());
		combat->set_defeat_effects(this->defeat_effects.get());

		combat->initialize();

		if (scope == game::get()->get_player_country()) {
			game::get()->set_current_combat(std::move(combat));
		} else {
			QTimer::singleShot(0, [combat = std::move(combat)]() -> QCoro::Task<void> {
				co_await combat->start_coro();
			});
		}
	}

	virtual std::string get_assignment_string(const domain *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		assert_throw(ctx.party != nullptr);

		std::string str = std::format("Party{}{}:", !this->attacker && this->surprise ? " (surprised)" : "", this->to_hit_modifier != 0 ? std::format(" (To Hit {})", number::to_signed_string(this->to_hit_modifier)) : "");
		for (const character *party_character : ctx.party->get_characters()) {
			std::string character_class_string;
			const character_class *character_class = party_character->get_game_data()->get_character_class();
			if (character_class != nullptr) {
				character_class_string += std::format(" {} {}", character_class->get_name(), party_character->get_game_data()->get_level());
			}
			str += "\n" + std::string(indent + 1, '\t') + std::format("{} ({}{} HP {}/{})", party_character->get_full_name(), party_character->get_species()->get_name(), character_class_string, party_character->get_game_data()->get_hit_points(), party_character->get_game_data()->get_max_hit_points());
		}

		str += "\n" + std::string(indent, '\t') + std::format("Does combat against{}:", this->attacker && this->surprise ? " (surprised)" : "");

		for (const auto &[monster_type, quantity_variant] : this->enemy_counts) {
			int additional_quantity = 0;
			for (const std::unique_ptr<enemy> &enemy : this->enemies) {
				if (enemy->get_monster_type() == monster_type) {
					++additional_quantity;
				}
			}

			std::string quantity_string;
			if (std::holds_alternative<int>(quantity_variant)) {
				const int quantity = std::get<int>(quantity_variant) + additional_quantity;
				quantity_string = std::to_string(quantity);
			} else {
				dice quantity_dice = std::get<dice>(quantity_variant);
				quantity_dice.change_modifier(additional_quantity);
				quantity_string = quantity_dice.to_display_string();
			}

			str += "\n" + std::string(indent + 1, '\t') + quantity_string + "x" + monster_type->get_name();
		}

		for (const target_variant<const character> &enemy_character : this->enemy_characters) {
			const character *character = this->get_enemy_character(enemy_character, ctx);
			std::string character_class_string;
			const character_class *character_class = character->get_game_data()->get_character_class();
			if (character_class != nullptr) {
				character_class_string += std::format(" {} {}", character_class->get_name(), character->get_game_data()->get_level());
			}
			str += "\n" + std::string(indent + 1, '\t') + std::format("{} ({}{})", character->get_full_name(), character->get_species()->get_name(), character_class_string);
		}

		for (const auto &[object_type, quantity] : this->object_counts) {
			str += "\n" + std::string(indent + 1, '\t') + std::to_string(quantity) + "x" + object_type->get_name();
		}

		if (this->victory_effects != nullptr) {
			const std::string effects_string = this->victory_effects->get_effects_string(scope, ctx, indent + 1, prefix);
			if (!effects_string.empty()) {
				str += "\n" + std::string(indent, '\t') + "If victorious:\n" + effects_string;
			}
		}

		if (this->defeat_effects != nullptr) {
			const std::string effects_string = this->defeat_effects->get_effects_string(scope, ctx, indent + 1, prefix);
			if (!effects_string.empty()) {
				str += "\n" + std::string(indent, '\t') + "If defeated:\n" + effects_string;
			}
		}

		return str;
	}

	std::vector<const character *> get_enemy_characters(const read_only_context &ctx, std::vector<std::shared_ptr<character_reference>> &generated_characters, character_map<const effect_list<const domain> *> &character_kill_effects) const
	{
		std::vector<const character *> enemy_characters;

		for (const auto &[monster_type, quantity_variant] : this->enemy_counts) {
			int quantity = 0;
			if (std::holds_alternative<int>(quantity_variant)) {
				quantity = std::get<int>(quantity_variant);
			} else {
				const dice quantity_dice = std::get<dice>(quantity_variant);
				quantity = random::get()->roll_dice(quantity_dice);
			}

			for (int i = 0; i < quantity; ++i) {
				std::shared_ptr<character_reference> enemy_character = character::generate_temporary(monster_type, nullptr, nullptr, nullptr, 0, {});
				enemy_characters.push_back(enemy_character->get_character());
				generated_characters.push_back(enemy_character);
			}
		}

		for (const std::unique_ptr<enemy> &enemy : this->enemies) {
			std::shared_ptr<character_reference> enemy_character = character::generate_temporary(enemy->get_monster_type(), nullptr, nullptr, nullptr, enemy->get_hit_points(), enemy->get_items());
			enemy_characters.push_back(enemy_character->get_character());
			generated_characters.push_back(enemy_character);

			if (enemy->get_kill_effects() != nullptr) {
				character_kill_effects[enemy_character->get_character()] = enemy->get_kill_effects();
			}
		}

		for (const target_variant<const character> &enemy_character : this->enemy_characters) {
			enemy_characters.push_back(this->get_enemy_character(enemy_character, ctx));
		}

		return vector::shuffled(enemy_characters);
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
	bool surprise = false;
	int to_hit_modifier = 0;
	bool retreat_allowed = true;
	QSize map_size;
	data_entry_map<monster_type, std::variant<int, dice>> enemy_counts;
	std::vector<std::unique_ptr<enemy>> enemies;
	std::vector<target_variant<const character>> enemy_characters;
	std::vector<std::unique_ptr<object>> objects;
	data_entry_map<object_type, int> object_counts;
	std::unique_ptr<effect_list<const domain>> victory_effects;
	std::unique_ptr<effect_list<const domain>> defeat_effects;
};

}
