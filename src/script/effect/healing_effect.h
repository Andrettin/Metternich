#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/effect/effect.h"
#include "util/dice.h"
#include "util/random.h"
#include "util/string_conversion_util.h"

namespace metternich {

class healing_effect final : public effect<const character>
{
public:
	explicit healing_effect(const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		if (string::is_bool(value)) {
			this->healing = string::to_bool(value);
		} else {
			this->healing = dice(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "healing";
		return identifier;
	}

	virtual void do_assignment_effect(const character *scope) const override
	{
		int quantity = 0;

		if (std::holds_alternative<bool>(this->healing)) {
			const bool value = std::get<bool>(this->healing);
			if (value) {
				quantity = scope->get_game_data()->get_max_hit_points();
			} else {
				return;
			}
		} else {
			const dice healing_dice = std::get<dice>(this->healing);
			quantity = random::get()->roll_dice(healing_dice);
		}

		scope->get_game_data()->change_hit_points(quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		std::string quantity_string;
		if (std::holds_alternative<bool>(this->healing)) {
			const bool value = std::get<bool>(this->healing);
			if (value) {
				quantity_string = "all";
			} else {
				return {};
			}
		} else {
			const dice healing_dice = std::get<dice>(this->healing);
			quantity_string = healing_dice.to_display_string();
		}

		return std::format("Heal {} HP", quantity_string);
	}

private:
	std::variant<bool, dice> healing;
};

}
