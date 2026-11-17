#pragma once

#include "script/effect/effect.h"
#include "script/effect/scripted_effect.h"
#include "script/scope_traits.h"

namespace metternich {

template <typename scope_type>
class scripted_effect_base;

template <typename scope_type>
class scripted_effect_effect final : public effect<scope_type>
{
public:
	explicit scripted_effect_effect(const std::string &effect_identifier, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		this->scripted_effect = scope_traits_t<scope_type>::scripted_effect_type::get(effect_identifier);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "scripted_effect";
		return class_identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(scope_type *scope, context &ctx) const override
	{
		co_await this->scripted_effect->get_effects().do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		return this->scripted_effect->get_effects().get_effects_string(scope, ctx, indent, prefix, false);
	}

private:
	const scripted_effect_base<scope_type> *scripted_effect = nullptr;
};

}
