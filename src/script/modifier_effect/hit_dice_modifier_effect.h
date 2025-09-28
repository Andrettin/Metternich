#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/dice.h"

namespace metternich {

class hit_dice_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit hit_dice_modifier_effect(const std::string &value)
	{
		this->hit_dice = dice(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "hit_dice";
		return identifier;
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		if (multiplier > 0) {
			scope->get_game_data()->apply_hit_dice(this->hit_dice);
		} else if (multiplier < 0) {
			scope->get_game_data()->remove_hit_dice(this->hit_dice);
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Hit Points";
	}

	virtual std::string get_string(const character *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return std::format("{}: {}{}", this->get_base_string(scope), multiplier > 0 ? "+" : "-", this->hit_dice.to_display_string());
	}

private:
	dice hit_dice;
};

}
