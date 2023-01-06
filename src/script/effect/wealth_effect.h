#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/effect/effect.h"

namespace metternich {

class wealth_effect final : public effect<const country>
{
public:
	explicit wealth_effect(const std::string &value, const gsml_operator effect_operator) : effect(effect_operator)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "wealth";
		return identifier;
	}

	virtual void do_assignment_effect(const country *country) const override
	{
		country->get_game_data()->set_wealth(this->quantity);
	}

	virtual void do_addition_effect(const country *country) const override
	{
		country->get_game_data()->change_wealth(this->quantity);
	}

	virtual void do_subtraction_effect(const country *country) const override
	{
		country->get_game_data()->change_wealth(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set Wealth to " + std::to_string(this->quantity);
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + std::to_string(this->quantity) + " Wealth";
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + std::to_string(this->quantity) + " Wealth";
	}

private:
	int quantity = 0;
};

}
