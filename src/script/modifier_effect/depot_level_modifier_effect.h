#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class depot_level_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit depot_level_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "depot_level";
		return identifier;
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_depot_level((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Depot Level";
	}
};

}
