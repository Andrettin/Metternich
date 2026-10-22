#pragma once

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/effect/else_if_effect.h"
#include "script/effect/if_effect.h"

namespace metternich {

template <typename scope_type>
class else_effect final : public effect<scope_type>
{
public:
	explicit else_effect(const gsml_operator effect_operator, const effect<scope_type> *previous_effect)
		: effect<scope_type>(effect_operator)
	{
		if (const if_effect<scope_type> *if_effect = dynamic_cast<const metternich::if_effect<scope_type> *>(previous_effect)) {
			this->previous_effect = if_effect;
		} else if (const else_if_effect<scope_type> *else_if_effect = dynamic_cast<const metternich::else_if_effect<scope_type> *>(previous_effect)) {
			this->previous_effect = else_if_effect;
		} else {
			assert_throw(false);
		}
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

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(scope_type *scope, context &ctx) const override
	{
		if (std::holds_alternative<const if_effect<scope_type> *>(this->previous_effect)) {
			if (std::get<const if_effect<scope_type> *>(this->previous_effect)->get_conditions().check(scope, ctx)) {
				co_return;
			}
		} else if (std::holds_alternative<const else_if_effect<scope_type> *>(this->previous_effect)) {
			if (std::get<const else_if_effect<scope_type> *>(this->previous_effect)->get_conditions().check(scope, ctx)) {
				co_return;
			}
		} else {
			assert_throw(false);
		}

		co_await this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		if (std::holds_alternative<const if_effect<scope_type> *>(this->previous_effect)) {
			if (std::get<const if_effect<scope_type> *>(this->previous_effect)->get_conditions().check(scope, ctx)) {
				return std::string();
			}
		} else if (std::holds_alternative<const else_if_effect<scope_type> *>(this->previous_effect)) {
			if (std::get<const else_if_effect<scope_type> *>(this->previous_effect)->get_conditions().check(scope, ctx)) {
				return std::string();
			}
		} else {
			assert_throw(false);
		}

		return this->effects.get_effects_string(scope, ctx, indent, prefix, false);
	}

private:
	std::variant<const if_effect<scope_type> *, const else_if_effect<scope_type> *> previous_effect;
	effect_list<scope_type> effects;
};

}
