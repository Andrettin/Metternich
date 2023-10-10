#pragma once

#include "economy/commodity.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class capital_commodity_bonus_per_population_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit capital_commodity_bonus_per_population_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "capital_commodity_bonus_per_population";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_capital_commodity_bonus_per_population(this->commodity, this->value * multiplier);
	}

	virtual std::string get_base_string() const override
	{
		if (this->commodity->is_storable()) {
			return std::format("Capital {} Output per Population", this->commodity->get_name());
		} else {
			return std::format("Capital {} per Population", this->commodity->get_name());
		}
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
