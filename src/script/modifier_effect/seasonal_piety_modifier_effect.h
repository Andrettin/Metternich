#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/fractional_int.h"

namespace metternich {

class seasonal_piety_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit seasonal_piety_modifier_effect(const std::string &value)
	{
		this->quantity = centesimal_int(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "seasonal_piety";
		return identifier;
	}

	virtual void apply(const character *scope, const int multiplier) const override
	{
		scope->get_game_data()->change_seasonal_piety(this->quantity * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return "Seasonal Piety: " + (this->quantity * multiplier).to_signed_string();
	}

private:
	centesimal_int quantity;
};

}