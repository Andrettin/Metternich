#pragma once

#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
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

namespace metternich {

template <typename scope_type>
class battle_effect final : public effect<scope_type>
{
public:
	explicit battle_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
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
		} else if (key == "victorious_enemies_attack_province") {
			this->victorious_enemies_attack_province = string::to_bool(property.get_value());
		} else {
			effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "enemies") {
			scope.for_each_property([&](const gsml_property &property) {
				const std::string &key = property.get_key();
				const military_unit_type *military_unit_type = military_unit_type::get(key);

				const std::string &value = property.get_value();
				const int quantity = std::stoi(value);

				this->enemies[military_unit_type] = quantity;
			});
		} else if (tag == "victory") {
			this->victory_effects = std::make_unique<effect_list<scope_type>>();
			database::process_gsml_data(this->victory_effects, scope);
		} else if (tag == "defeat") {
			this->defeat_effects = std::make_unique<effect_list<scope_type>>();
			database::process_gsml_data(this->defeat_effects, scope);
		} else {
			effect<scope_type>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		std::vector<qunique_ptr<military_unit>> enemy_unit_unique_ptrs;
		std::vector<military_unit *> enemy_units;

		for (const auto &[military_unit_type, quantity] : this->enemies) {
			for (int i = 0; i < quantity; ++i) {
				auto military_unit = make_qunique<metternich::military_unit>(military_unit_type);
				enemy_units.push_back(military_unit.get());
				enemy_unit_unique_ptrs.push_back(std::move(military_unit));
			}
		}

		auto enemy_army = make_qunique<army>(enemy_units, std::monostate());

		bool success = false;

		if (this->attacker) {
			success = game::get()->do_battle(ctx.attacking_army, enemy_army.get());
		} else {
			success = !game::get()->do_battle(enemy_army.get(), ctx.defending_army);
		}

		if (success) {
			if (this->victory_effects != nullptr) {
				this->victory_effects->do_effects(scope, ctx);
			}
		} else {
			if (this->defeat_effects != nullptr) {
				this->defeat_effects->do_effects(scope, ctx);
			}

			if (this->victorious_enemies_attack_province) {
				//FIXME: make it so the enemies attack the province where the battle is taking place
			}
		}
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = "Battle against:";

		for (const auto &[military_unit_type, quantity] : this->enemies) {
			str += "\n" + std::string(indent + 1, '\t') + std::to_string(quantity) + "x" + military_unit_type->get_name();
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

private:
	bool attacker = false;
	bool victorious_enemies_attack_province = false;
	military_unit_type_map<int> enemies;
	std::unique_ptr<effect_list<scope_type>> victory_effects;
	std::unique_ptr<effect_list<scope_type>> defeat_effects;
};

}
