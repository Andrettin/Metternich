#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"

namespace metternich {

class entrenchment_bonus_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit entrenchment_bonus_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "entrenchment_bonus_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		const int change = (this->value * multiplier).to_int();

		scope->get_game_data()->change_entrenchment_bonus_modifier(change);
	}

	virtual std::string get_base_string() const override
	{
		return "Entrenchment Bonus";
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
