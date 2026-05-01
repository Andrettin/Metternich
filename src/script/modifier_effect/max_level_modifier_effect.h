#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class max_level_modifier_effect final : public modifier_effect<const province>
{
public:
	explicit max_level_modifier_effect(const std::string &value)
		: modifier_effect<const province>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "max_level";
		return identifier;
	}

	virtual void apply(const province *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_max_level((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const province *scope) const override
	{
		Q_UNUSED(scope);

		return "Max Level";
	}
};

}
