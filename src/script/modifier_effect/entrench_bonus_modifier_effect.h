#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"

namespace metternich {

class entrench_bonus_modifier_effect final : public modifier_effect<military_unit>
{
public:
	explicit entrench_bonus_modifier_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "entrench_bonus_modifier";
		return identifier;
	}

	virtual void apply(military_unit *scope, const centesimal_int &multiplier) const override
	{
		scope->change_entrench_bonus_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Entrenchment Bonus";
	}

	virtual int get_score() const override
	{
		return this->value;
	}
};

}
