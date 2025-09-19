#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "script/condition/numerical_condition.h"
#include "util/centesimal_int.h"

namespace metternich {

class population_scaled_commodity_condition final : public numerical_condition<domain, read_only_context, centesimal_int>
{
public:
	explicit population_scaled_commodity_condition(const metternich::commodity *commodity, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<domain, read_only_context, centesimal_int>(value, condition_operator), commodity(commodity)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "population_scaled_commodity";
		return class_identifier;
	}

	virtual int get_scope_value(const domain *scope) const override
	{
		return scope->get_economy()->get_stored_commodity(this->commodity);
	}

	virtual int get_value(const domain *scope) const override
	{
		return (this->get_base_value() * scope->get_game_data()->get_population_unit_count()).to_int();
	}

	virtual std::string get_value_name() const override
	{
		return std::format("{} per population unit", this->commodity->get_name());
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
