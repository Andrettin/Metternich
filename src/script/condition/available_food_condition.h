#pragma once

#include "script/condition/numerical_condition.h"

namespace metternich {

class available_food_condition final : public numerical_condition<country>
{
public:
	explicit available_food_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<country>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "available_food";
		return class_identifier;
	}

	virtual int get_scope_value(const country *scope) const override
	{
		return scope->get_game_data()->get_available_food();
	}

	virtual std::string get_value_name() const override
	{
		return "Available Food";
	}
};

}
