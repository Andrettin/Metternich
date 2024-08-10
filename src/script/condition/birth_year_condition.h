#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class birth_year_condition final : public numerical_condition<character>
{
public:
	explicit birth_year_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "birth_year";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return scope->get_birth_date().year();
	}

	virtual std::string get_value_name() const override
	{
		return "birth year";
	}
};

}
