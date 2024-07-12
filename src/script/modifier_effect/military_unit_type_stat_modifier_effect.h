#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "util/string_util.h"

namespace metternich {

class military_unit_type_stat_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit military_unit_type_stat_modifier_effect(const metternich::military_unit_type *military_unit_type, const military_unit_stat stat, const std::string &value)
		: modifier_effect(value), military_unit_type(military_unit_type), stat(stat)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "military_unit_type_stat_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_military_unit_type_stat_modifier(this->military_unit_type, this->stat, this->value * multiplier);
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} {}", this->military_unit_type->get_name(), get_military_unit_stat_name(this->stat));
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
	const metternich::military_unit_type *military_unit_type = nullptr;
	military_unit_stat stat{};
};

}
