#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

class naval_morale_resistance_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit naval_morale_resistance_modifier_effect(const std::string &value)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "naval_morale_resistance";
		return identifier;
	}

	virtual void apply(const country *scope, const int multiplier) const override
	{
		scope->get_game_data()->change_naval_morale_resistance_modifier(this->quantity * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return "Naval Morale Resistance: " + number::to_signed_string(this->quantity * multiplier) + "%";
	}

private:
	int quantity = 0;
};

}
