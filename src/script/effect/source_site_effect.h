#pragma once

#include "script/effect/scope_effect.h"

namespace metternich {

class site;

template <typename upper_scope_type>
class source_site_effect final : public scope_effect<upper_scope_type, const site>
{
public:
	explicit source_site_effect(const gsml_operator effect_operator)
		: scope_effect<upper_scope_type, const site>(effect_operator)
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

	virtual const site *get_scope(const upper_scope_type *upper_scope, context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		if (std::holds_alternative<const site *>(ctx.source_scope)) {
			return std::get<const site *>(ctx.source_scope);
		}

		return nullptr;
	}
};

}
