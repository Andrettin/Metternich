#pragma once

#include "script/effect/scope_effect_base.h"
#include "util/string_util.h"

namespace metternich {

template <typename upper_scope_type, typename scope_type>
class scope_effect : public scope_effect_base<upper_scope_type, scope_type>
{
public:
	explicit scope_effect(const gsml_operator effect_operator)
		: scope_effect_base<upper_scope_type, scope_type>(effect_operator)
	{
	}

	virtual std::string get_scope_name(const upper_scope_type *upper_scope, const read_only_context &ctx) const override final
	{
		const scope_type *scope = this->get_effects_string_scope(upper_scope, ctx);

		if (scope == nullptr) {
			return std::string();
		}

		return string::highlight(scope->get_scope_name());
	}

	virtual scope_type *get_scope(const upper_scope_type *upper_scope) const
	{
		Q_UNUSED(upper_scope);

		return nullptr;
	}

	virtual const scope_type *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const
	{
		Q_UNUSED(ctx);

		return this->get_scope(upper_scope);
	}

	virtual scope_type *get_scope(const upper_scope_type *upper_scope, context &ctx) const
	{
		Q_UNUSED(ctx);

		return this->get_scope(upper_scope);
	}

	virtual void do_assignment_effect(upper_scope_type *upper_scope, context &ctx) const override final
	{
		scope_type *new_scope = this->get_scope(upper_scope, ctx);

		if (new_scope == nullptr) {
			return;
		}

		this->do_scope_effect(new_scope, upper_scope, ctx);
	}

	virtual const scope_type *get_effects_string_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override final
	{
		return this->get_scope(upper_scope, ctx);
	}
};

}
