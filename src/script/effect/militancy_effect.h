#pragma once

#include "population/population_unit.h"
#include "script/effect/effect.h"
#include "util/fractional_int.h"

namespace metternich {

class militancy_effect final : public effect<population_unit>
{
public:
	explicit militancy_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<population_unit>(effect_operator)
	{
		this->quantity = centesimal_int(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "militancy";
		return identifier;
	}

	virtual void do_assignment_effect(population_unit *scope) const override
	{
		scope->set_militancy(this->quantity);
	}

	virtual void do_addition_effect(population_unit *scope) const override
	{
		scope->change_militancy(this->quantity);
	}

	virtual void do_subtraction_effect(population_unit *scope) const override
	{
		scope->change_militancy(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set " + string::highlight("Militancy") + " to " + this->quantity.to_string();
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + this->quantity.to_string() + " " + string::highlight("Militancy");
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + this->quantity.to_string() + " " + string::highlight("Militancy");
	}

private:
	centesimal_int quantity;
};

}
