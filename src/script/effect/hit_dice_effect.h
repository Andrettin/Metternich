#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/effect/effect.h"
#include "util/dice.h"

namespace metternich {

class hit_dice_effect final : public effect<const character>
{
public:
	explicit hit_dice_effect(const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		this->hit_dice = dice(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "hit_dice";
		return identifier;
	}

	virtual void do_addition_effect(const character *scope) const override
	{
		scope->get_game_data()->apply_hit_dice(this->hit_dice);
	}

	virtual std::string get_addition_string() const override
	{
		return std::format("Hit Points: +{}", this->hit_dice.to_display_string());
	}

private:
	dice hit_dice;
};

}
