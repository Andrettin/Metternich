#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"

namespace metternich {

class melee_modifier_effect final : public modifier_effect<military_unit>
{
public:
	explicit melee_modifier_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "melee";
		return identifier;
	}

	virtual void apply(military_unit *scope, const centesimal_int &multiplier) const override
	{
		scope->change_melee((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Melee";
	}

	virtual int get_score() const override
	{
		return this->value * 10;
	}
};

}
