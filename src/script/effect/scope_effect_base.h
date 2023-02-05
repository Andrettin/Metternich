#pragma once

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"

namespace metternich {

template <typename upper_scope_type, typename scope_type>
class scope_effect_base : public effect<upper_scope_type>
{
public:
	explicit scope_effect_base(const gsml_operator effect_operator) : effect<upper_scope_type>(effect_operator)
	{
		if (effect_operator != gsml_operator::assignment) {
			throw std::runtime_error("Scope effects can only have the assignment operator as their operator.");
		}
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		this->effects.process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->effects.process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		this->effects.check();
	}

	void do_scope_effect(scope_type *scope, context &ctx) const
	{
		if (scope == nullptr) {
			return;
		}

		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_scope_name() const = 0;

	virtual std::string get_conditions_string(const size_t indent) const
	{
		Q_UNUSED(indent);

		return std::string();
	}

	virtual const scope_type *get_effects_string_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const
	{
		Q_UNUSED(upper_scope);
		Q_UNUSED(ctx);

		return nullptr;
	}

	virtual std::string get_assignment_string(const upper_scope_type *upper_scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override final
	{
		std::string str = this->get_scope_name() + ":";

		const std::string conditions_str = this->get_conditions_string(indent + 2);
		if (!conditions_str.empty()) {
			str += "\n" + std::string(indent + 1, '\t') + "Conditions:\n" + conditions_str;
		}

		const std::string effects_str = this->effects.get_effects_string(this->get_effects_string_scope(upper_scope, ctx), ctx, indent + 1, prefix);
		if (!effects_str.empty()) {
			str += "\n" + effects_str;
		}

		return str;
	}

private:
	effect_list<scope_type> effects;
};

}
