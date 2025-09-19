#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class province_count_condition final : public numerical_condition<domain, read_only_context>
{
public:
	explicit province_count_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<domain, read_only_context>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "province_count";
		return class_identifier;
	}

	virtual int get_scope_value(const domain *scope) const override
	{
		return scope->get_game_data()->get_province_count();
	}

	virtual std::string get_value_name() const override
	{
		return "Province Count";
	}
};

}
