#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

class unit_upgrade_cost_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit unit_upgrade_cost_modifier_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "unit_upgrade_cost_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_unit_upgrade_cost_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Unit Upgrade Cost";
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