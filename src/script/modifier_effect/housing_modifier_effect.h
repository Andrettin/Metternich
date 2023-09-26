#pragma once

#include "economy/commodity.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class housing_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit housing_modifier_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "housing";
		return identifier;
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_housing((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Housing";
	}

	virtual int get_score() const override
	{
		return this->value;
	}

private:
	int value = 0;
};

}
