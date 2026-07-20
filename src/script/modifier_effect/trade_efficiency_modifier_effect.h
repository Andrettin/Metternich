#pragma once

#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class trade_efficiency_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit trade_efficiency_modifier_effect(const std::string &value) : modifier_effect<scope_type>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "trade_efficiency_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		if constexpr (std::is_same_v<scope_type, const domain>) {
			scope->get_economy()->change_trade_efficiency_modifier((this->value * multiplier).to_int());
		} else {
			scope->get_game_data()->change_trade_efficiency_modifier((this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return "Trade Efficiency";
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
