#pragma once

#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class industrial_output_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit industrial_output_modifier_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "industrial_output_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_industrial_output_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Industrial Output";
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual int get_score() const override
	{
		return this->value;
	}
};

}