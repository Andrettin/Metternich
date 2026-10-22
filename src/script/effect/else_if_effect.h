#pragma once

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/effect/if_effect.h"

namespace metternich {

template <typename scope_type>
class else_if_effect final : public effect<scope_type>
{
public:
	explicit else_if_effect(const gsml_operator effect_operator, const effect<scope_type> *previous_effect)
		: effect<scope_type>(effect_operator)
	{
		if (const if_effect<scope_type> *if_effect = dynamic_cast<const metternich::if_effect<scope_type> *>(previous_effect)) {
			this->previous_effect = if_effect;
		} else if (const else_if_effect<scope_type> *else_if_effect = dynamic_cast<const metternich::else_if_effect<scope_type> *>(previous_effect)) {
			this->previous_effect = else_if_effect;
		} else {
			throw std::runtime_error("\"else_if\" effects must have a preceding \"if\" or \"else_if\" effect.");
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "else_if";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property & property) override
	{
		this->effects.process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data & scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			this->conditions.process_gsml_data(scope);
		} else {
			this->effects.process_gsml_scope(scope);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(scope_type *scope, context &ctx) const override
	{
		if (this->is_previous_branch_taken(scope, ctx)) {
			co_return;
		}

		if (!this->conditions.check(scope, ctx)) {
			co_return;
		}

		co_await this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string & prefix) const override
	{
		if (this->is_previous_branch_taken(scope, ctx)) {
			return std::string();
		}

		if (!this->conditions.check(scope, ctx)) {
			return std::string();
		}

		return this->effects.get_effects_string(scope, ctx, indent, prefix, false);
	}

	bool is_previous_branch_taken(const scope_type *scope, const read_only_context &ctx) const
	{
		return std::visit([&](const auto *previous_effect) {
			return previous_effect->is_branch_taken(scope, ctx);
		}, this->previous_effect);
	}

	bool is_branch_taken(const scope_type *scope, const read_only_context &ctx) const
	{
		return this->is_previous_branch_taken(scope, ctx) || this->conditions.check(scope, ctx);
	}

private:
	std::variant<const if_effect<scope_type> *, const else_if_effect<scope_type> *> previous_effect;
	and_condition<std::remove_const_t<scope_type>> conditions;
	effect_list<scope_type> effects;
};

}
