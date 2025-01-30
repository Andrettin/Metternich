#pragma once

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/effect/if_effect.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class else_effect final : public effect<scope_type>
{
public:
	explicit else_effect(const gsml_operator effect_operator, const effect<scope_type> *previous_effect)
		: effect<scope_type>(effect_operator)
	{
		this->if_effect = dynamic_cast<const metternich::if_effect<scope_type> *>(previous_effect);
		assert_throw(this->if_effect != nullptr);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "else";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		this->effects.process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->effects.process_gsml_scope(scope);
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		if (this->if_effect->get_conditions().check(scope, ctx)) {
			return;
		}

		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		if (this->if_effect->get_conditions().check(scope, ctx)) {
			return std::string();
		}

		return this->effects.get_effects_string(scope, ctx, indent, prefix, false);
	}

private:
	const metternich::if_effect<scope_type> *if_effect = nullptr;
	effect_list<scope_type> effects;
};

}
