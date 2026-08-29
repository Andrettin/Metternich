#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class base_population_capacity_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit base_population_capacity_modifier_effect(const std::string &value)
		: modifier_effect<const site>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "base_population_capacity";
		return identifier;
	}

	virtual void apply(const site *scope, const decimillesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_base_population_capacity((this->value * multiplier).to_int64());
	}

	virtual std::string get_base_string(const site *scope) const override
	{
		Q_UNUSED(scope);

		return "Base Population Capacity";
	}
};

}
