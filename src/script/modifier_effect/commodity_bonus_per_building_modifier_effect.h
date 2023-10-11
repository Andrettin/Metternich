#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "infrastructure/building_type.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class commodity_bonus_per_building_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_bonus_per_building_modifier_effect(const metternich::commodity *commodity, const metternich::building_type *building, const std::string &value)
		: modifier_effect<scope_type>(value), commodity(commodity), building(building)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus_per_building";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_building_commodity_bonus(this->building, this->commodity, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} per {}", this->commodity->get_name(), this->building->get_name());
	}

private:
	const metternich::commodity *commodity = nullptr;
	const metternich::building_type *building = nullptr;
};

}
