#pragma once

#include "character/character_reference.h"
#include "character/monster_type.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "game/battle.h"
#include "game/game.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "unit/army.h"
#include "unit/military_unit.h"
#include "unit/military_unit_type.h"
#include "unit/military_unit_type_container.h"
#include "util/qunique_ptr.h"
#include "util/string_conversion_util.h"
#include "util/vector_random_util.h"

namespace metternich {

using enemy_type_variant = std::variant<const military_unit_type *, const monster_type *>;

struct enemy_type_compare final
{
	bool operator()(const enemy_type_variant &lhs, const enemy_type_variant &rhs) const
	{
		if (lhs.index() != rhs.index()) {
			return lhs.index() < rhs.index();
		}

		if (std::holds_alternative<const military_unit_type *>(lhs)) {
			return std::get<const military_unit_type *>(lhs)->get_identifier() < std::get<const military_unit_type *>(rhs)->get_identifier();
		} else {
			return std::get<const monster_type *>(lhs)->get_identifier() < std::get<const monster_type *>(rhs)->get_identifier();
		}
	}
};

class enemy final
{
public:
	explicit enemy(const gsml_data &scope)
	{
		this->military_unit_type = military_unit_type::get(scope.get_tag());

		scope.for_each_element([this](const gsml_property &property) {
			if (property.get_key() == "monster_type") {
				this->monster_type = monster_type::get(property.get_value());
			} else if (property.get_key() == "health") {
				this->health = std::stoi(property.get_value());
			} else if (property.get_key() == "placement") {
				this->placement = magic_enum::enum_cast<combat_placement>(property.get_value()).value();
			} else {
				assert_throw(false);
			}
		}, [this](const gsml_data &child_scope) {
			if (child_scope.get_tag() == "placement_offset") {
				this->placement_offset = child_scope.to_point();
			} else if (child_scope.get_tag() == "items") {
				for (const std::string &value : child_scope.get_values()) {
					this->items.push_back(item_type::get(value));
				}

				child_scope.for_each_property([this](const gsml_property &property) {
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

	const metternich::military_unit_type *get_military_unit_type() const
	{
		return this->military_unit_type;
	}

	const metternich::monster_type *get_monster_type() const
	{
		return this->monster_type;
	}

	int get_health()
	{
		return this->health;
	}

	const combat_placement get_placement() const
	{
		return this->placement;
	}

	const QPoint &get_placement_offset() const
	{
		return this->placement_offset;
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
	const metternich::military_unit_type *military_unit_type = nullptr;
	const metternich::monster_type *monster_type = nullptr;
	int health = 0;
	combat_placement placement = combat_placement::right;
	QPoint placement_offset = QPoint(0, 0);
	std::vector<const item_type *> items;
	std::unique_ptr<effect_list<const domain>> kill_effects;
};

class battle_effect final : public effect<const domain>
{
public:
	explicit battle_effect(const gsml_operator effect_operator) : effect<const domain>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "battle";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();

		if (key == "attacker") {
			this->attacker = string::to_bool(property.get_value());
		} else if (key == "defender_neutral") {
			this->defender_neutral = string::to_bool(property.get_value());
		} else if (key == "surprise") {
			this->surprise = string::to_bool(property.get_value());
		} else if (key == "to_hit_modifier") {
			this->to_hit_modifier = std::stoi(property.get_value());
		} else if (key == "retreat_allowed") {
			this->retreat_allowed = string::to_bool(property.get_value());
		} else if (key == "victorious_enemies_attack_province") {
			this->victorious_enemies_attack_province = string::to_bool(property.get_value());
		} else {
			effect<const domain>::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "enemies") {
			scope.for_each_element([this](const gsml_property &property) {
				const std::string &key = property.get_key();
				const military_unit_type *military_unit_type = military_unit_type::get(key);

				const std::string &value = property.get_value();
				if (string::is_number(value)) {
					this->enemy_counts[military_unit_type] = std::stoi(value);
				} else {
					this->enemy_counts[military_unit_type] = dice(value);
				}
			}, [this](const gsml_data &child_scope) {
				auto enemy = std::make_unique<metternich::enemy>(child_scope);
				if (enemy->get_monster_type() != nullptr) {
					if (!this->enemy_counts.contains(enemy->get_monster_type())) {
						this->enemy_counts[enemy->get_monster_type()] = 0;
					}
				} else if (!this->enemy_counts.contains(enemy->get_military_unit_type())) {
					this->enemy_counts[enemy->get_military_unit_type()] = 0;
				}
				this->enemies.push_back(std::move(enemy));
			});
		} else if (tag == "on_victory") {
			this->victory_effects = std::make_unique<effect_list<const domain>>();
			this->victory_effects->process_gsml_data(scope);
		} else if (tag == "on_defeat") {
			this->defeat_effects = std::make_unique<effect_list<const domain>>();
			this->defeat_effects->process_gsml_data(scope);
		} else {
			effect<const domain>::process_gsml_scope(scope);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(const domain *scope, context &ctx) const override
	{
		std::vector<military_unit *> enemy_units;

		std::vector<std::shared_ptr<character_reference>> generated_characters;
		character_map<const enemy *> character_enemy_infos;
		const std::vector<qunique_ptr<military_unit>> enemy_unit_unique_ptrs = co_await this->create_enemy_units(ctx, generated_characters, character_enemy_infos);

		for (const auto &enemy_unit : enemy_unit_unique_ptrs) {
			enemy_units.push_back(enemy_unit.get());
		}

		auto enemy_army = make_qunique<army>(enemy_units, std::monostate());

		qunique_ptr<battle> battle;

		if (this->attacker) {
			ctx.attacking_army = ctx.army;
			ctx.defending_army = enemy_army.get();
			battle = make_qunique<metternich::battle>(ctx.army, enemy_army.get(), QSize());
		} else {
			ctx.attacking_army = enemy_army.get();
			ctx.defending_army = ctx.army;
		}

		battle = make_qunique<metternich::battle>(ctx.attacking_army, ctx.defending_army, QSize());

		battle->set_defender_neutral(this->defender_neutral);
		battle->set_surprise(this->surprise);
		battle->set_attacker_to_hit_modifier(this->attacker ? this->to_hit_modifier : 0);
		battle->set_defender_to_hit_modifier(this->attacker ? 0 : this->to_hit_modifier);
		battle->set_attacker_retreat_allowed(this->attacker ? this->retreat_allowed : false);
		battle->set_defender_retreat_allowed(this->attacker ? false : this->retreat_allowed);

		const domain *scope_domain = effect<const domain>::get_scope_domain(scope);
		assert_throw(scope_domain != nullptr);

		battle->set_scope(scope_domain);
		context battle_ctx = ctx;
		battle_ctx.in_combat = true;
		battle->set_context(battle_ctx);
		battle->set_victory_effects(this->victory_effects.get());
		battle->set_defeat_effects(this->defeat_effects.get());

		co_await battle->initialize();

		QFuture<bool> success_future = battle->get_future();

		if (scope_domain == game::get()->get_player_domain() ) {
			game::get()->set_current_combat(std::move(battle));
		} else {
			QTimer::singleShot(0, [battle = std::move(battle)]() -> QCoro::Task<void> {
				co_await battle->start_coro();
			});
		}

		const bool success = co_await success_future;

		enemy_army->clear();

		if (!success) {
			if (this->victorious_enemies_attack_province) {
				//FIXME: make it so the enemies attack the province where the battle is taking place
			}
		}
	}

	virtual std::string get_assignment_string(const domain *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = std::format("Army{}:", !this->attacker && this->surprise ? " (surprised)" : "");
		for (const auto &[military_unit_type, quantity] : ctx.army->get_military_unit_type_counts()) {
			str += "\n" + std::string(indent + 1, '\t') + std::format("{}x{}{}", quantity, military_unit_type->get_name(), this->to_hit_modifier != 0 && military_unit_type->is_character() ? std::format(" (To Hit {})", number::to_signed_string(this->to_hit_modifier)) : "");
		}

		str += "\n" + std::string(indent, '\t') + std::format("Battles against{}{}:", this->attacker && this->defender_neutral ? " (neutral until attacked)" : "", this->attacker && this->surprise ? " (surprised)" : "");

		for (const auto &[enemy_type_variant, quantity_variant] : this->enemy_counts) {
			int additional_quantity = 0;
			const military_unit_type *military_unit_type = std::holds_alternative<const metternich::military_unit_type *>(enemy_type_variant) ? std::get<const metternich::military_unit_type *>(enemy_type_variant) : nullptr;
			const monster_type *monster_type = std::holds_alternative<const metternich::monster_type *>(enemy_type_variant) ? std::get<const metternich::monster_type *>(enemy_type_variant) : nullptr;
			for (const std::unique_ptr<enemy> &enemy : this->enemies) {
				if ((enemy->get_military_unit_type() != nullptr && enemy->get_military_unit_type() == military_unit_type) || (enemy->get_monster_type() != nullptr && enemy->get_monster_type() == monster_type)) {
					++additional_quantity;
				}
			}

			const std::string &enemy_type_name = monster_type != nullptr ? monster_type->get_name() : military_unit_type->get_name();

			std::string quantity_string;
			if (std::holds_alternative<int>(quantity_variant)) {
				const int quantity = std::get<int>(quantity_variant) + additional_quantity;
				quantity_string = std::to_string(quantity);
			} else {
				dice quantity_dice = std::get<dice>(quantity_variant);
				quantity_dice.change_modifier(additional_quantity);
				quantity_string = quantity_dice.to_display_string();
			}

			str += "\n" + std::string(indent + 1, '\t') + quantity_string + "x" + enemy_type_name;
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

	[[nodiscard]] QCoro::Task<std::vector<qunique_ptr<military_unit>>> create_enemy_units(const read_only_context &ctx, std::vector<std::shared_ptr<character_reference>> &generated_characters, character_map<const enemy *> &character_enemy_infos) const
	{
		std::vector<qunique_ptr<military_unit>> enemy_units;
		std::map<std::string, int> used_name_counts;

		for (const auto &[enemy_type_variant, quantity_variant] : this->enemy_counts) {
			int quantity = 0;
			if (std::holds_alternative<int>(quantity_variant)) {
				quantity = std::get<int>(quantity_variant);
			} else {
				const dice quantity_dice = std::get<dice>(quantity_variant);
				quantity = random::get()->roll_dice(quantity_dice);
			}

			const military_unit_type *military_unit_type = std::holds_alternative<const metternich::military_unit_type *>(enemy_type_variant) ? std::get<const metternich::military_unit_type *>(enemy_type_variant) : nullptr;
			const monster_type *monster_type = std::holds_alternative<const metternich::monster_type *>(enemy_type_variant) ? std::get<const metternich::monster_type *>(enemy_type_variant) : nullptr;

			for (int i = 0; i < quantity; ++i) {
				qunique_ptr<military_unit> military_unit;
				if (military_unit_type != nullptr) {
					military_unit = co_await metternich::military_unit::create(military_unit_type);
				} else {
					//FIXME: get the monster military unit type
					assert_throw(false);
					std::shared_ptr<character_reference> enemy_character = co_await character::generate_temporary(monster_type, nullptr, nullptr, nullptr, 0, {});
					generated_characters.push_back(enemy_character);

					military_unit = co_await metternich::military_unit::create(military_unit_type, nullptr, enemy_character->get_character());
				}

				enemy_units.push_back(std::move(military_unit));
			}
		}

		for (const std::unique_ptr<enemy> &enemy : this->enemies) {
			qunique_ptr<military_unit> military_unit;

			if (enemy->get_monster_type() != nullptr) {
				assert_throw(enemy->get_military_unit_type()->is_monster());

				std::shared_ptr<character_reference> enemy_character = co_await character::generate_temporary(enemy->get_monster_type(), nullptr, nullptr, nullptr, enemy->get_health(), enemy->get_items());
				generated_characters.push_back(enemy_character);
				character_enemy_infos[enemy_character->get_character()] = enemy.get();

				military_unit = co_await metternich::military_unit::create(enemy->get_military_unit_type(), nullptr, enemy_character->get_character());
			} else {
				military_unit = co_await metternich::military_unit::create(enemy->get_military_unit_type());
			}

			enemy_units.push_back(std::move(military_unit));
		}

		co_return enemy_units;
	}

private:
	bool attacker = false;
	bool defender_neutral = false;
	bool surprise = false;
	int to_hit_modifier = 0;
	bool retreat_allowed = true;
	bool victorious_enemies_attack_province = false;
	std::map<enemy_type_variant, std::variant<int, dice>, enemy_type_compare> enemy_counts;
	std::vector<std::unique_ptr<enemy>> enemies;
	std::unique_ptr<effect_list<const domain>> victory_effects;
	std::unique_ptr<effect_list<const domain>> defeat_effects;
};

}
