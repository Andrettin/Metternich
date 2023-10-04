#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/effect/effect.h"

namespace metternich {

class inflation_effect final : public effect<const country>
{
public:
	explicit inflation_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<const country>(effect_operator)
	{
		this->quantity = centesimal_int(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "inflation";
		return identifier;
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		scope->get_game_data()->set_inflation(this->quantity);
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		scope->get_game_data()->change_inflation(this->quantity);
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		scope->get_game_data()->change_inflation(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Set {} to {}%", string::highlight("Inflation"), this->quantity.to_string());
	}

	virtual std::string get_addition_string() const override
	{
		return std::format("Gain {}% {}", this->quantity.to_string(), string::highlight("Inflation"));
	}

	virtual std::string get_subtraction_string() const override
	{
		return std::format("Lose {}% {}", this->quantity.to_string(), string::highlight("Inflation"));
	}

private:
	centesimal_int quantity;
};

}
