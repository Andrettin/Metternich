#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class highest_production_capacity_condition final : public numerical_condition<site, read_only_context>
{
public:
	explicit highest_production_capacity_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<site, read_only_context>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "highest_production_capacity";
		return class_identifier;
	}

	virtual int get_scope_value(const site *scope) const override
	{
		return scope->get_game_data()->get_highest_production_capacity();
	}

	virtual std::string get_value_name() const override
	{
		return "Highest Production Capacity";
	}
};

}
