#pragma once

#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class literacy_rate_condition final : public numerical_condition<scope_type, read_only_context, centesimal_int>
{
public:
	explicit literacy_rate_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type, read_only_context, centesimal_int>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "literacy_rate";
		return class_identifier;
	}

	virtual centesimal_int get_scope_value(const scope_type *scope) const override
	{
		if constexpr (std::is_same_v<scope_type, population_unit>) {
			return centesimal_int(scope->get_literacy_rate());
		} else {
			return scope->get_game_data()->get_population()->get_literacy_rate();
		}
	}

	virtual std::string get_value_name() const override
	{
		return "Literacy";
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
