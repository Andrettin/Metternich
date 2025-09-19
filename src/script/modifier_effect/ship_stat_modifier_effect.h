#pragma once

#include "domain/country_military.h"
#include "domain/domain.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "unit/transporter_stat.h"
#include "unit/transporter_type.h"
#include "util/string_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

class ship_stat_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit ship_stat_modifier_effect(const std::string &stat_name, const std::string &value)
		: modifier_effect(value), military_unit_stat(magic_enum::enum_cast<metternich::military_unit_stat>(stat_name).value()), transporter_stat(magic_enum::enum_cast<metternich::transporter_stat>(stat_name).value())
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "ship_stat_modifier";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		for (const military_unit_type *military_unit_type : military_unit_type::get_all()) {
			if (military_unit_type->is_ship()) {
				scope->get_military()->change_military_unit_type_stat_modifier(military_unit_type, this->military_unit_stat, this->value * multiplier);
			}
		}

		for (const transporter_type *transporter_type : transporter_type::get_all()) {
			if (transporter_type->is_ship()) {
				scope->get_game_data()->change_transporter_type_stat_modifier(transporter_type, this->transporter_stat, this->value * multiplier);
			}
		}
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("Ship {}", get_military_unit_stat_name(this->military_unit_stat));
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return is_percent_military_unit_stat(this->military_unit_stat);
	}

private:
	metternich::military_unit_stat military_unit_stat{};
	metternich::transporter_stat transporter_stat{};
};

}
