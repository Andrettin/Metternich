#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class movement_cost_modifier_effect final : public modifier_effect<const province>
{
public:
	explicit movement_cost_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "movement_cost_modifier";
		return identifier;
	}

	virtual void apply(const province *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_movement_cost_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const province *scope) const override
	{
		Q_UNUSED(scope);

		return "Movement Cost";
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_negative(const centesimal_int &multiplier) const override
	{
		return (this->value * multiplier) > 0;
	}
};

}
