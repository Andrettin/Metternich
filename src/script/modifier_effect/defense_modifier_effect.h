#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"

namespace metternich {

class defense_modifier_effect final : public modifier_effect<military_unit>
{
public:
	explicit defense_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "defense";
		return identifier;
	}

	virtual void apply(military_unit *scope, const centesimal_int &multiplier) const override
	{
		scope->change_defense((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Defense";
	}
};

}
