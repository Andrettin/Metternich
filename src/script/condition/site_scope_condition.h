#pragma once

#include "population/population_unit.h"
#include "script/condition/scope_condition.h"

namespace metternich {

class site;

class site_scope_condition final : public scope_condition<population_unit, site, read_only_context, condition<site>>
{
public:
	explicit site_scope_condition(const gsml_operator condition_operator)
		: scope_condition<population_unit, site, read_only_context, condition<site>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "site_scope";
		return class_identifier;
	}

	virtual const site *get_scope(const population_unit *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return upper_scope->get_site();
	}

	virtual std::string get_scope_name() const override
	{
		return "Site";
	}
};

}
