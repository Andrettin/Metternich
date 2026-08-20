#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "script/modifier_effect/scripted_modifier_effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class scripted_modifier_effect_base;

template <typename scope_type>
class scripted_modifier_effect_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit scripted_modifier_effect_modifier_effect(const std::string &modifier_effect_identifier)
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			this->scripted_modifier_effect = character_scripted_modifier_effect::get(modifier_effect_identifier);
		} else if constexpr (std::is_same_v<scope_type, const domain>) {
			this->scripted_modifier_effect = domain_scripted_modifier_effect::get(modifier_effect_identifier);
		} else if constexpr (std::is_same_v<scope_type, military_unit>) {
			this->scripted_modifier_effect = military_unit_scripted_modifier_effect::get(modifier_effect_identifier);
		} else if constexpr (std::is_same_v<scope_type, const province>) {
			this->scripted_modifier_effect = province_scripted_modifier_effect::get(modifier_effect_identifier);
		} else {
			assert_throw(false);
		}
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "scripted_modifier_effect";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(scope_type *scope, const decimillesimal_int &multiplier) const override
	{
		co_await this->scripted_modifier_effect->get_modifier().apply(scope, multiplier);
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return {};
	}

	virtual std::string get_string(const scope_type *scope, const decimillesimal_int &multiplier, const size_t indent, const bool ignore_decimals, const std::string &separator) const override
	{
		return this->scripted_modifier_effect->get_modifier().get_string(scope, multiplier, indent, ignore_decimals, separator);
	}

private:
	const scripted_modifier_effect_base<scope_type> *scripted_modifier_effect = nullptr;
};

}
