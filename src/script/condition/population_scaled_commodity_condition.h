#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "script/condition/numerical_condition.h"
#include "util/fractional_int.h"

namespace metternich {

class population_scaled_commodity_condition final : public numerical_condition<country, centesimal_int>
{
public:
	explicit population_scaled_commodity_condition(const metternich::commodity *commodity, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<country, centesimal_int>(value, condition_operator), commodity(commodity)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "population_scaled_commodity";
		return class_identifier;
	}

	virtual int get_scope_value(const country *scope) const override
	{
		return scope->get_game_data()->get_stored_commodity(this->commodity);
	}

	virtual int get_value(const country *scope) const override
	{
		return (this->get_base_value() * scope->get_game_data()->get_total_unit_count()).to_int();
	}

	virtual std::string get_value_name() const override
	{
		return std::format("{} per population unit", this->commodity->get_name());
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
