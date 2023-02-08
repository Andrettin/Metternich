#pragma once

#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class prestige_condition final : public numerical_condition<scope_type>
{
public:
	explicit prestige_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "prestige";
		return class_identifier;
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		if constexpr (std::is_same_v<scope_type, character>) {
			if (scope->get_game_data()->is_ruler()) {
				return scope->get_game_data()->get_employer()->get_game_data()->get_prestige_int();
			}
		}

		return scope->get_game_data()->get_prestige_int();
	}

	virtual std::string get_value_name() const override
	{
		return "Prestige";
	}
};

}
