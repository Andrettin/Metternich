#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/scope_condition.h"

namespace metternich {

class site;

class capital_scope_condition final : public scope_condition<domain, site, read_only_context, condition<site>>
{
public:
	explicit capital_scope_condition(const gsml_operator condition_operator)
		: scope_condition<domain, site, read_only_context, condition<site>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "capital_scope";
		return class_identifier;
	}

	virtual const site *get_scope(const domain *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return upper_scope->get_game_data()->get_capital();
	}

	virtual std::string get_scope_name() const override
	{
		return "Capital";
	}
};

}
