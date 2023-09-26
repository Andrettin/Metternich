#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class artillery_cost_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit artillery_cost_modifier_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "artillery_cost_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_artillery_cost_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Artillery Cost";
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual int get_score() const override
	{
		return -this->value;
	}
};

}
