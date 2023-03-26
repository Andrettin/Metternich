#pragma once

#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class wealth_condition final : public numerical_condition<scope_type>
{
public:
	explicit wealth_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "wealth";
		return class_identifier;
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		return scope->get_game_data()->get_wealth();
	}

	virtual std::string get_value_name() const override
	{
		return "Wealth";
	}
};

}
