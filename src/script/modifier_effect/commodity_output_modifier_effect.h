#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class commodity_output_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_output_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect<scope_type>(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_output_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_commodity_output_modifier(this->commodity, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} Output", this->commodity->get_name());
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
