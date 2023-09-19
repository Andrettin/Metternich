#pragma once

#include "script/condition/scope_condition.h"

namespace metternich {

class province;

template <typename upper_scope_type>
class province_condition final : public scope_condition<upper_scope_type, province>
{
public:
	explicit province_condition(const gsml_operator condition_operator)
		: scope_condition<upper_scope_type, province>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "province";
		return class_identifier;
	}

	virtual const province *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return condition<upper_scope_type>::get_scope_province(upper_scope);
	}

	virtual std::string get_scope_name() const override
	{
		return "Province";
	}
};

}
