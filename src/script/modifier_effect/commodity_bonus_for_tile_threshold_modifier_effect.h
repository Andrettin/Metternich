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
class commodity_bonus_for_tile_threshold_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_bonus_for_tile_threshold_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: commodity(commodity)
	{
		this->threshold = std::stoi(value);
		this->value = 1;
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
			return std::format("{} per Tile Producing at Least {} {}", number::to_signed_string(this->value), this->commodity->get_name(), this->threshold, this->commodity->get_name());
		} else {
			return std::format("{} per {}-Producing Tile", this->commodity->get_name(), this->commodity->get_name());
		}
	}

	virtual int get_score() const override
	{
		return this->value * 10 / this->threshold;
	}

private:
	const metternich::commodity *commodity = nullptr;
	int threshold = 1;
};

}
