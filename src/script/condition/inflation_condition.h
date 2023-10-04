#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class inflation_condition final : public numerical_condition<country, centesimal_int>
{
public:
	explicit inflation_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<country, centesimal_int>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "inflation";
		return class_identifier;
	}

	virtual int get_scope_value(const country *scope) const override
	{
		return scope->get_game_data()->get_inflation().to_int();
	}

	virtual std::string get_value_name() const override
	{
		return "Inflation";
	}
};

}
