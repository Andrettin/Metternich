#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class commodity_output_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_output_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: commodity(commodity)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_output_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const int multiplier) const override
	{
		scope->get_game_data()->change_commodity_output_modifier(this->commodity, this->quantity * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return this->commodity->get_name() + " Production: " + number::to_signed_string(this->quantity * multiplier) + "%";
	}

private:
	const metternich::commodity *commodity = nullptr;
	int quantity = 0;
};

}