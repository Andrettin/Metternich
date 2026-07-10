#pragma once

#include "script/condition/scope_condition.h"

namespace metternich {

class domain;

class overlord_scope_condition final : public scope_condition<domain, domain, read_only_context, condition<domain>>
{
public:
	explicit overlord_scope_condition(const gsml_operator condition_operator)
		: scope_condition<domain, domain, read_only_context, condition<domain>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "overlord";
		return class_identifier;
	}

	virtual const domain *get_scope(const domain *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return upper_scope->get_game_data()->get_overlord();
	}

	virtual std::string get_scope_name() const override
	{
		return "Overlord";
	}
};

}
