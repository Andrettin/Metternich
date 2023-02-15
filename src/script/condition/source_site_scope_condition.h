#pragma once

#include "script/condition/scope_condition.h"

namespace metternich {

class site;

template <typename upper_scope_type>
class source_site_scope_condition final : public scope_condition<upper_scope_type, site>
{
public:
	explicit source_site_scope_condition(const gsml_operator condition_operator)
		: scope_condition<upper_scope_type, site>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "source_site";
		return class_identifier;
	}

	virtual const site *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		if (std::holds_alternative<const site *>(ctx.source_scope)) {
			return std::get<const site *>(ctx.source_scope);
		}

		return nullptr;
	}

	virtual std::string get_scope_name() const override
	{
		return "Source Site";
	}
};

}
