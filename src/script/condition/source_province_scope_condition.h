#pragma once

#include "script/condition/scope_condition.h"

namespace metternich {

class province;

template <typename upper_scope_type>
class source_province_scope_condition final : public scope_condition<upper_scope_type, province, read_only_context, condition<province>>
{
public:
	explicit source_province_scope_condition(const gsml_operator condition_operator = gsml_operator::assignment)
		: scope_condition<upper_scope_type, province, read_only_context, condition<province>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "source_province";
		return class_identifier;
	}

	virtual const province *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		if (std::holds_alternative<const province *>(ctx.source_scope)) {
			return std::get<const province *>(ctx.source_scope);
		}

		return nullptr;
	}

	virtual std::string get_scope_name() const override
	{
		return "Source Province";
	}
};

}
