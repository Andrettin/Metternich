#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class fortification_level_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit fortification_level_modifier_effect(const std::string &value)
		: modifier_effect<const site>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "fortification_level";
		return identifier;
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_fortification_level(this->value * multiplier);
	}

	virtual std::string get_base_string(const site *scope) const override
	{
		Q_UNUSED(scope);

		return "Fortification Level";
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}
};

}
