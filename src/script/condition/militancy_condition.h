#pragma once

#include "population/population_unit.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class militancy_condition final : public numerical_condition<population_unit>
{
public:
	explicit militancy_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<population_unit>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "militancy";
		return class_identifier;
	}

	virtual int get_scope_value(const population_unit *scope) const override
	{
		return scope->get_militancy().to_int();
	}

	virtual std::string get_value_name() const override
	{
		return "Militancy";
	}
};

}