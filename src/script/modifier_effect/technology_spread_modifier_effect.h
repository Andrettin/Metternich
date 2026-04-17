#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class technology_spread_modifier_effect final : public modifier_effect<const province>
{
public:
	explicit technology_spread_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "technology_spread_modifier";
		return identifier;
	}

	virtual void apply(const province *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_technology_spread_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const province *scope) const override
	{
		Q_UNUSED(scope);

		return "Technology Spread";
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
