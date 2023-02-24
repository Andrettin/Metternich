#pragma once

#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

class population_type_condition final : public condition<population_unit>
{
public:
	explicit population_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<population_unit>(condition_operator)
	{
		this->population_type = population_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "population_type";
		return class_identifier;
	}

	virtual bool check_assignment(const population_unit *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_type() == this->population_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(this->population_type->get_name());
	}

private:
	const metternich::population_type *population_type = nullptr;
};

}
