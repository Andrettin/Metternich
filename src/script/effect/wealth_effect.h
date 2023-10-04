#pragma once

#include "script/effect/effect.h"

namespace metternich {

template <typename scope_type>
class wealth_effect final : public effect<scope_type>
{
public:
	explicit wealth_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "wealth";
		return identifier;
	}

	virtual void do_assignment_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->set_wealth(this->quantity);
	}

	virtual void do_addition_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->change_wealth(this->quantity);
	}

	virtual void do_subtraction_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->change_wealth(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Set {} to {}", string::highlight("Wealth"), this->quantity);
	}

	virtual std::string get_addition_string() const override
	{
		return std::format("Gain {} {}", this->quantity, string::highlight("Wealth"));
	}

	virtual std::string get_subtraction_string() const override
	{
		return std::format("Lose {} {}", this->quantity, string::highlight("Wealth"));
	}

private:
	int quantity = 0;
};

}
