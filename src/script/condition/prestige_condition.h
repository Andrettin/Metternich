#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class prestige_condition final : public numerical_condition<character>
{
public:
	explicit prestige_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "prestige";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return scope->get_game_data()->get_prestige_int();
	}

	virtual const char *get_value_name() const override
	{
		return "Prestige";
	}
};

}