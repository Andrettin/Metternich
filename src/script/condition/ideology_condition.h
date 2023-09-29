#pragma once

#include "country/ideology.h"
#include "population/population_unit.h"
#include "script/condition/condition.h"

namespace metternich {

class ideology_condition final : public condition<population_unit>
{
public:
	explicit ideology_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<population_unit>(condition_operator)
	{
		this->ideology = ideology::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "ideology";
		return class_identifier;
	}

	virtual bool check_assignment(const population_unit *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_ideology() == this->ideology;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(this->ideology->get_name()) + " ideology";
	}

private:
	const metternich::ideology *ideology = nullptr;
};

}
