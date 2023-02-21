#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"
#include "util/number_util.h"

namespace metternich {

class defense_modifier_effect final : public modifier_effect<military_unit>
{
public:
	explicit defense_modifier_effect(const std::string &value)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "defense";
		return identifier;
	}

	virtual void apply(military_unit *scope, const int multiplier) const override
	{
		scope->change_defense(this->quantity * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return "Defense: " + number::to_signed_string(this->quantity * multiplier);
	}

private:
	int quantity = 0;
};

}
