#pragma once

#include "domain/country_economy.h"
#include "domain/domain.h"
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
		if (!this->commodity->is_enabled()) {
			return;
		}

		if constexpr (std::is_same_v<scope_type, const domain>) {
			scope->get_economy()->change_commodity_bonus_for_tile_threshold(this->commodity, (this->value * multiplier).to_int(), this->threshold);
		} else {
			scope->get_game_data()->change_commodity_bonus_for_tile_threshold(this->commodity, (this->value * multiplier).to_int(), this->threshold);
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		if (this->threshold > 1) {
			return std::format("{} per Tile Producing at Least {} {}", this->commodity->get_name(), this->threshold, this->commodity->get_name());
		} else {
			return std::format("{} per {}-Producing Tile", this->commodity->get_name(), this->commodity->get_name());
		}
	}

	virtual bool is_hidden(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return !this->commodity->is_enabled();
	}

private:
	const metternich::commodity *commodity = nullptr;
	int threshold = 1;
};

}
