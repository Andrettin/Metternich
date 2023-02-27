#pragma once

#include "script/effect/scope_effect.h"

namespace metternich {

template <typename upper_scope_type>
class location_effect final : public scope_effect<upper_scope_type, const province>
{
public:
	explicit location_effect(const gsml_operator effect_operator)
		: scope_effect<upper_scope_type, const province>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "location";
		return class_identifier;
	}

	virtual const province *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return effect<upper_scope_type>::get_scope_province(upper_scope);
	}

	virtual const province *get_scope(const upper_scope_type *upper_scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		return effect<upper_scope_type>::get_scope_province(upper_scope);
	}
};

}
