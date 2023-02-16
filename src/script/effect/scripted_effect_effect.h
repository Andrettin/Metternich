#pragma once

#include "script/effect/effect.h"
#include "script/effect/scripted_effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class scripted_effect_base;

template <typename scope_type>
class scripted_effect_effect final : public effect<scope_type>
{
public:
	explicit scripted_effect_effect(const std::string &effect_identifier, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			this->scripted_effect = character_scripted_effect::get(effect_identifier);
		} else if constexpr (std::is_same_v<scope_type, const country>) {
			this->scripted_effect = country_scripted_effect::get(effect_identifier);
		} else if constexpr (std::is_same_v<scope_type, population_unit>) {
			this->scripted_effect = population_unit_scripted_effect::get(effect_identifier);
		} else if constexpr (std::is_same_v<scope_type, const province>) {
			this->scripted_effect = province_scripted_effect::get(effect_identifier);
		} else {
			assert_throw(false);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "scripted_effect";
		return class_identifier;
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		this->scripted_effect->get_effects().do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		return this->scripted_effect->get_effects().get_effects_string(scope, ctx, indent, prefix, false);
	}

private:
	const scripted_effect_base<scope_type> *scripted_effect = nullptr;
};

}
