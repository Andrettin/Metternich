#pragma once

#include "population/population_unit.h"
#include "script/condition/scope_condition.h"

namespace metternich {

class site;

class settlement_condition final : public scope_condition<population_unit, site>
{
public:
	explicit settlement_condition(const gsml_operator condition_operator)
		: scope_condition<population_unit, site>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "settlement";
		return class_identifier;
	}

	virtual const site *get_scope(const population_unit *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return upper_scope->get_settlement();
	}

	virtual std::string get_scope_name() const override
	{
		return "Settlement";
	}
};

}
