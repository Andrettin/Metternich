#pragma once

#include "population/population_unit.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class consciousness_condition final : public numerical_condition<scope_type, read_only_context>
{
public:
	explicit consciousness_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type, read_only_context>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "consciousness";
		return class_identifier;
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		if constexpr (std::is_same_v<scope_type, population_unit>) {
			return scope->get_consciousness().to_int();
		} else {
			return scope->get_game_data()->get_population()->get_average_consciousness().to_int();
		}
	}

	virtual std::string get_value_name() const override
	{
		return "Consciousness";
	}
};

}
