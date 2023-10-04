#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class wealth_inflated_condition final : public numerical_condition<country>
{
public:
	explicit wealth_inflated_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<country>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "wealth_inflated";
		return class_identifier;
	}

	virtual int get_scope_value(const country *scope) const override
	{
		return scope->get_game_data()->get_wealth();
	}

	virtual int get_value(const country *scope) const override
	{
		return scope->get_game_data()->get_inflated_value(this->get_base_value());
	}

	virtual std::string get_value_name() const override
	{
		return "Wealth";
	}
};

}
