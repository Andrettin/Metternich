#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"
#include "unit/military_unit_stat.h"

namespace metternich {

class military_unit_stat_modifier_effect final : public modifier_effect<military_unit>
{
public:
	explicit military_unit_stat_modifier_effect(const military_unit_stat stat, const std::string &value)
		: modifier_effect(value), stat(stat)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "military_unit_stat";
		return identifier;
	}

	virtual void apply(military_unit *scope, const centesimal_int &multiplier) const override
	{
		scope->change_stat(this->stat, this->value * multiplier);
	}

	virtual std::string get_base_string() const override
	{
		return std::string(get_military_unit_stat_name(this->stat));
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
