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

	void do_scope_effect(scope_type *scope, upper_scope_type *upper_scope, context &ctx) const
	{
		if (scope == nullptr) {
			return;
		}

		ctx.previous_scope = upper_scope;

		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_scope_name() const
	{
		return std::string();
	}

	virtual std::string get_scope_name(const upper_scope_type *upper_scope, const read_only_context &ctx) const
	{
		Q_UNUSED(upper_scope);
		Q_UNUSED(ctx);

		return this->get_scope_name();
	}

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
		const std::string scope_name = this->get_scope_name(upper_scope, ctx);

		if (scope_name.empty()) {
			return std::string();
		}

		std::string str = scope_name + ":";

		const scope_type *scope = this->get_effects_string_scope(upper_scope, ctx);
		size_t scope_indent = indent + 1;
		if constexpr (std::is_same_v<scope_type, const country>) {
			if (ctx.is_root_scope(upper_scope) && scope == effect<upper_scope_type>::get_scope_country(upper_scope) && indent == 0) {
				scope_indent = indent;
				str.clear();
			}
		}

		const std::string conditions_str = this->get_conditions_string(scope_indent + 1);
		if (!conditions_str.empty()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::string(scope_indent, '\t') + "Conditions:\n" + conditions_str;
		}

		read_only_context lower_ctx = ctx;
		lower_ctx.previous_scope = upper_scope;

		const std::string effects_str = this->effects.get_effects_string(scope, lower_ctx, scope_indent, prefix);
		if (!effects_str.empty()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += effects_str;
		}

		return str;
	}

private:
	effect_list<scope_type> effects;
};

}
