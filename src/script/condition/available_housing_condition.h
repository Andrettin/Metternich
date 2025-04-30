#pragma once

#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class available_housing_condition final : public numerical_condition<scope_type, read_only_context>
{
public:
	explicit available_housing_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type, read_only_context>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "available_housing";
		return class_identifier;
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		return scope->get_game_data()->get_available_housing().to_int();
	}

	virtual std::string get_value_name() const override
	{
		return "Available Housing";
	}
};

}
