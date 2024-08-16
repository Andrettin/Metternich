#pragma once

#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class available_health_condition final : public numerical_condition<scope_type>
{
public:
	explicit available_health_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "available_health";
		return class_identifier;
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		return scope->get_game_data()->get_available_health().to_int();
	}

	virtual std::string get_value_name() const override
	{
		return "Available Health";
	}
};

}
