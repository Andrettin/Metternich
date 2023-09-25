#pragma once

#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class militancy_condition final : public numerical_condition<scope_type>
{
public:
	explicit militancy_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "militancy";
		return class_identifier;
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		if constexpr (std::is_same_v<scope_type, population_unit>) {
			return scope->get_militancy().to_int();
		} else {
			return scope->get_game_data()->get_population()->get_average_militancy().to_int();
		}
	}

	virtual std::string get_value_name() const override
	{
		return "Militancy";
	}
};

}
