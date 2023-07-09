#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"
#include "util/number_util.h"

namespace metternich {

class bonus_vs_fortifications_modifier_effect final : public modifier_effect<military_unit>
{
public:
	explicit bonus_vs_fortifications_modifier_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "bonus_vs_fortifications";
		return identifier;
	}

	virtual void apply(military_unit *scope, const centesimal_int &multiplier) const override
	{
		scope->change_bonus_vs_fortifications((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Bonus vs. Fortifications";
	}

	virtual int get_score() const override
	{
		return this->value;
	}
};

}
