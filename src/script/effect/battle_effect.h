#pragma once

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

namespace metternich {

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
			scope.for_each_property([this](const gsml_property &property) {
				const std::string &key = property.get_key();
				const military_unit_type *military_unit_type = military_unit_type::get(key);

				const std::string &value = property.get_value();
				const int quantity = std::stoi(value);

				this->enemies[military_unit_type] = quantity;
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
		std::vector<qunique_ptr<military_unit>> enemy_unit_unique_ptrs;
		std::vector<military_unit *> enemy_units;

		for (const auto &[military_unit_type, quantity] : this->enemies) {
			for (int i = 0; i < quantity; ++i) {
				auto military_unit = co_await metternich::military_unit::create(military_unit_type);
				enemy_units.push_back(military_unit.get());
				enemy_unit_unique_ptrs.push_back(std::move(military_unit));
			}
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
	bool defender_neutral = false;
	bool surprise = false;
	int to_hit_modifier = 0;
	bool retreat_allowed = true;
	bool victorious_enemies_attack_province = false;
	military_unit_type_map<int> enemies;
	std::unique_ptr<effect_list<const domain>> victory_effects;
	std::unique_ptr<effect_list<const domain>> defeat_effects;
};

}
