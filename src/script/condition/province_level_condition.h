#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class province_level_condition final : public numerical_condition<province, read_only_context>
{
public:
	explicit province_level_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<province, read_only_context>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "province_level";
		return class_identifier;
	}

	virtual int get_scope_value(const province *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_level();
	}

	virtual std::string get_value_name() const override
	{
		return "Province Level";
	}
};

}
