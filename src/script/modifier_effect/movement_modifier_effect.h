#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"

namespace metternich {

class movement_modifier_effect final : public modifier_effect<military_unit>
{
public:
	explicit movement_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "movement";
		return identifier;
	}

	virtual void apply(military_unit *scope, const centesimal_int &multiplier) const override
	{
		scope->change_movement((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Movement";
	}
};

}
