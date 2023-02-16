#pragma once

#include "script/context.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class save_scope_as_effect final : public effect<scope_type>
{
public:
	explicit save_scope_as_effect(const std::string &value, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		this->scope_name = value;
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "save_scope_as";
		return class_identifier;
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		ctx.get_saved_scopes<scope_type>()[this->scope_name] = scope;
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	std::string scope_name;
};

}
