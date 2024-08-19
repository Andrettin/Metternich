#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "unit/transporter_stat.h"
#include "unit/transporter_type.h"
#include "util/string_util.h"

namespace metternich {

class merchant_ship_stat_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit merchant_ship_stat_modifier_effect(const transporter_stat stat, const std::string &value)
		: modifier_effect(value), stat(stat)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "merchant_ship_stat_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		for (const transporter_type *transporter_type : transporter_type::get_all()) {
			if (transporter_type->is_ship()) {
				scope->get_game_data()->change_transporter_type_stat_modifier(transporter_type, this->stat, this->value * multiplier);
			}
		}
	}

	virtual std::string get_base_string() const override
	{
		return std::format("Merchant Ship {}", get_transporter_stat_name(this->stat));
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return is_percent_transporter_stat(this->stat);
	}

private:
	transporter_stat stat{};
};

}
