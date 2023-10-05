#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class commodity_bonus_for_tile_threshold_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_bonus_for_tile_threshold_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: commodity(commodity)
	{
		this->threshold = std::stoi(value);
		this->value = centesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus_for_tile_threshold";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_commodity_bonus_for_tile_threshold(this->commodity, (this->value * multiplier).to_int(), this->threshold);
	}

	virtual std::string get_base_string() const override
	{
		if (this->threshold > 1) {
			return std::format("{} per Tile Producing at Least {} {}", this->commodity->get_name(), this->threshold, this->commodity->get_name());
		} else {
			return std::format("{} per {}-Producing Tile", this->commodity->get_name(), this->commodity->get_name());
		}
	}

private:
	const metternich::commodity *commodity = nullptr;
	int threshold = 1;
};

}
