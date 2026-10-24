#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "util/string_util.h"

namespace metternich {

class merchant_ship_stat_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit merchant_ship_stat_modifier_effect(const military_unit_stat stat, const std::string &value)
		: modifier_effect(value), stat(stat)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "merchant_ship_stat_modifier";
		return identifier;
	}

	virtual void apply(const domain *scope, const decimillesimal_int &multiplier) const override
	{
		for (const military_unit_type *military_unit_type : military_unit_type::get_all()) {
			if (military_unit_type->is_ship() && military_unit_type->is_transporter()) {
				scope->get_military()->change_military_unit_type_stat_modifier(military_unit_type, this->stat, centesimal_int(this->value * multiplier));
			}
		}
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("Merchant Ship {}", get_military_unit_stat_name(this->stat));
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return is_percent_military_unit_stat(this->stat);
	}

private:
	military_unit_stat stat{};
};

}
