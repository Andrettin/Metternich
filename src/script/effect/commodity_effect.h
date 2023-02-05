#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "script/effect/effect.h"

namespace metternich {

class commodity_effect final : public effect<const country>
{
public:
	explicit commodity_effect(const metternich::commodity *commodity, const std::string &value, const gsml_operator effect_operator)
		: effect<const country>(effect_operator), commodity(commodity)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "commodity";
		return identifier;
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		scope->get_game_data()->set_stored_commodity(this->commodity, this->quantity);
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		scope->get_game_data()->change_stored_commodity(this->commodity, this->quantity);
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		scope->get_game_data()->change_stored_commodity(this->commodity, -this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set " + this->commodity->get_name()  + " to " + std::to_string(this->quantity);
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + std::to_string(this->quantity) + " " + this->commodity->get_name();
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + std::to_string(this->quantity) + " " + this->commodity->get_name();
	}

private:
	const metternich::commodity *commodity = nullptr;
	int quantity = 0;
};

}
