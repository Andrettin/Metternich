#pragma once

#include "script/condition/and_condition.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class if_effect final : public effect<scope_type>
{
public:
	explicit if_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "if";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		this->effects.process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			database::process_gsml_data(this->conditions, scope);
		} else {
			this->effects.process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		if (!this->conditions.check(scope, ctx)) {
			return;
		}

		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		if (!this->conditions.check(scope, ctx)) {
			return std::string();
		}

		return this->effects.get_effects_string(scope, ctx, indent, prefix, false);
	}

private:
	and_condition<std::remove_const_t<scope_type>> conditions;
	effect_list<scope_type> effects;
};

}
