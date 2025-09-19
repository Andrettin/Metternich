#pragma once

#include "script/condition/scope_condition.h"

namespace metternich {

class domain;

template <typename upper_scope_type>
class country_scope_condition final : public scope_condition<upper_scope_type, domain, read_only_context, condition<domain>>
{
public:
	explicit country_scope_condition(const gsml_operator condition_operator)
		: scope_condition<upper_scope_type, domain, read_only_context, condition<domain>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "country";
		return class_identifier;
	}

	virtual const domain *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return condition<upper_scope_type>::get_scope_country(upper_scope);
	}

	virtual std::string get_scope_name() const override
	{
		return "Domain";
	}
};

}
