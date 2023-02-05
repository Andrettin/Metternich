#pragma once

#include "script/effect/effect.h"
#include "util/fractional_int.h"

namespace metternich {

template <typename scope_type>
class prestige_effect final : public effect<scope_type>
{
public:
	explicit prestige_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		this->quantity = centesimal_int(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "prestige";
		return identifier;
	}

	virtual void do_assignment_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->set_prestige(this->quantity);
	}

	virtual void do_addition_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->change_prestige(this->quantity);
	}

	virtual void do_subtraction_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->change_prestige(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set Prestige to " + this->quantity.to_string();
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + this->quantity.to_string() + " Prestige";
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + this->quantity.to_string() + " Prestige";
	}

private:
	centesimal_int quantity;
};

}
