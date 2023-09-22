#pragma once

#include "script/effect/scope_effect.h"

namespace metternich {

template <typename upper_scope_type, typename scope_type>
class saved_scope_effect final : public scope_effect<upper_scope_type, scope_type>
{
public:
	explicit saved_scope_effect(const gsml_operator effect_operator)
		: scope_effect<upper_scope_type, scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "saved_scope";
		return class_identifier;
	}

	virtual const scope_type *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const
	{
		Q_UNUSED(upper_scope);

		return ctx.get_saved_scope<const scope_type>(this->scope_name);
	}

	virtual scope_type *get_scope(const upper_scope_type *upper_scope, context &ctx) const
	{
		Q_UNUSED(upper_scope);

		return ctx.get_saved_scope<scope_type>(this->scope_name);
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		if (property.get_key() == "scope") {
			this->scope_name = property.get_value();
		} else {
			scope_effect<upper_scope_type, scope_type>::process_gsml_property(property);
		}
	}

private:
	std::string scope_name;
};

}
