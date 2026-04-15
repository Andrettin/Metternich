#pragma once

#include "population/population.h"
#include "population/population_type.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class population_type_percent_condition final : public numerical_condition<scope_type, read_only_context, centesimal_int>
{
public:
	explicit population_type_percent_condition(const metternich::population_type *population_type, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type, read_only_context, centesimal_int>(value, condition_operator), population_type(population_type)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "population_type_percent";
		return class_identifier;
	}

	virtual centesimal_int get_scope_value(const scope_type *scope) const override
	{
		return centesimal_int(scope->get_game_data()->get_population()->get_type_size(this->population_type)) * 100 / scope->get_game_data()->get_population()->get_size();
	}

	virtual std::string get_value_name() const override
	{
		return this->population_type->get_name();
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	const metternich::population_type *population_type = nullptr;
};

}
