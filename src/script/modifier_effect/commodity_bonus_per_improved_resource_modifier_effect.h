#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class commodity_bonus_per_improved_resource_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_bonus_per_improved_resource_modifier_effect(const metternich::commodity *commodity, const metternich::resource *resource, const std::string &value)
		: modifier_effect<scope_type>(value), commodity(commodity), resource(resource)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus_per_improved_resource";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_improved_resource_commodity_bonus(this->resource, this->commodity, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} per Improved {}", this->commodity->get_name(), this->resource->get_name());
	}

private:
	const metternich::commodity *commodity = nullptr;
	const metternich::resource *resource = nullptr;
};

}
