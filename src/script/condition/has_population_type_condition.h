#pragma once

#include "population/population.h"
#include "population/population_type.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class has_population_type_condition final : public condition<scope_type>
{
public:
	explicit has_population_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->type = population_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_population_type";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_population()->get_type_counts().contains(this->type);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Has {} population type", string::highlight(this->type->get_name()));
	}

private:
	const population_type *type = nullptr;
};

}
