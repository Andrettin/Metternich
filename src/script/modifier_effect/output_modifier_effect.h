#pragma once

#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class output_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit output_modifier_effect(const std::string &value) : modifier_effect<scope_type>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "output_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_output_modifier(this->value * multiplier);
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return "Output";
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
