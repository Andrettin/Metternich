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
		if constexpr (std::is_same_v<scope_type, const character>) {
			if (scope->get_game_data()->is_ruler()) {
				//wealth changes for rulers affect their countries instead
				scope->get_game_data()->get_employer()->get_game_data()->set_wealth(this->quantity);
				return;
			}
		}

		scope->get_game_data()->set_wealth(this->quantity);
	}

	virtual void do_addition_effect(const scope_type *scope) const override
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			if (scope->get_game_data()->is_ruler()) {
				scope->get_game_data()->get_employer()->get_game_data()->change_wealth(this->quantity);
				return;
			}
		}

		scope->get_game_data()->change_wealth(this->quantity);
	}

	virtual void do_subtraction_effect(const scope_type *scope) const override
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			if (scope->get_game_data()->is_ruler()) {
				scope->get_game_data()->get_employer()->get_game_data()->change_wealth(-this->quantity);
				return;
			}
		}

		scope->get_game_data()->change_wealth(-this->quantity);
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
