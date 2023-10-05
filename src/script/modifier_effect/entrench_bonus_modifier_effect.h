#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"

namespace metternich {

template <typename scope_type>
class entrench_bonus_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit entrench_bonus_modifier_effect(const std::string &value) : modifier_effect<scope_type>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "entrench_bonus_modifier";
		return identifier;
	}

	virtual void apply(scope_type *scope, const centesimal_int &multiplier) const override
	{
		const int change = (this->value * multiplier).to_int();

		if constexpr (std::is_same_v<scope_type, military_unit>) {
			scope->change_entrench_bonus_modifier(change);
		} else {
			scope->get_game_data()->change_entrench_bonus_modifier(change);
		}
	}

	virtual std::string get_base_string() const override
	{
		return "Entrenchment Bonus";
	}

	virtual bool is_percent() const override
	{
		return std::is_same_v<scope_type, const country>;
	}
};

}
