#pragma once

#include "script/condition/scope_condition_base.h"

namespace metternich {

template <typename upper_scope_type, typename scope_type>
class scope_condition : public scope_condition_base<upper_scope_type, scope_type>
{
public:
	explicit scope_condition(const gsml_operator condition_operator)
		: scope_condition_base<upper_scope_type, scope_type>(condition_operator)
	{
	}

	virtual const scope_type *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const = 0;

	virtual bool check_assignment(const upper_scope_type *upper_scope, const read_only_context &ctx) const override final
	{
		const scope_type *scope = this->get_scope(upper_scope, ctx);

		if (scope == nullptr) {
			return false;
		}

		return this->check_scope(scope, ctx);
	}
};

}
