#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/effect/effect.h"
#include "util/dice.h"
#include "util/random.h"

namespace metternich {

class damage_effect final : public effect<const character>
{
public:
	explicit damage_effect(const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		if (string::is_number(value)) {
			this->damage = std::stoi(value);
		} else {
			this->damage = dice(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "damage";
		return identifier;
	}

	virtual void do_assignment_effect(const character *scope) const override
	{
		int damage = 0;

		if (std::holds_alternative<int>(this->damage)) {
			damage = std::get<int>(this->damage);
		} else {
			const dice damage_dice = std::get<dice>(this->damage);
			damage = random::get()->roll_dice(damage_dice);
		}

		scope->get_game_data()->change_hit_points(-damage);
	}

	virtual std::string get_assignment_string() const override
	{
		std::string quantity_string;
		if (std::holds_alternative<int>(this->damage)) {
			quantity_string = std::to_string(std::get<int>(this->damage));
		} else {
			const dice damage_dice = std::get<dice>(this->damage);
			quantity_string = damage_dice.to_display_string();
		}

		return std::format("Receive {} Damage", quantity_string);
	}

private:
	std::variant<int, dice> damage;
};

}
