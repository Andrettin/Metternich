#pragma once

#include "country/country.h"
#include "country/country_economy.h"
#include "economy/commodity.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class commodity_condition final : public numerical_condition<country, read_only_context>
{
public:
	explicit commodity_condition(const metternich::commodity *commodity, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<country, read_only_context>(commodity->string_to_value(value), condition_operator), commodity(commodity)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "commodity";
		return class_identifier;
	}

	virtual int get_scope_value(const country *scope) const override
	{
		return scope->get_economy()->get_stored_commodity(this->commodity);
	}

	virtual std::string get_value_name() const override
	{
		return this->commodity->get_name();
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
