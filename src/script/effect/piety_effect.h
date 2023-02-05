#pragma once

#include "script/effect/effect.h"
#include "util/fractional_int.h"

namespace metternich {

template <typename scope_type>
class piety_effect final : public effect<const scope_type>
{
public:
	explicit piety_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		this->quantity = centesimal_int(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "piety";
		return identifier;
	}

	virtual void do_assignment_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->set_piety(this->quantity);
	}

	virtual void do_addition_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->change_piety(this->quantity);
	}

	virtual void do_subtraction_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->change_piety(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set Piety to " + this->quantity.to_string();
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + this->quantity.to_string() + " Piety";
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + this->quantity.to_string() + " Piety";
	}

private:
	centesimal_int quantity;
};

}
