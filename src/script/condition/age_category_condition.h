#pragma once

#include "character/age_category.h"
#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/numerical_condition.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

class age_category_condition final : public numerical_condition<character, read_only_context>
{
public:
	explicit age_category_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character, read_only_context>(static_cast<int>(magic_enum::enum_cast<age_category>(value).value()), condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "age_category";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return static_cast<int>(scope->get_game_data()->get_age_category());
	}

	virtual std::string get_base_value_string() const override
	{
		return std::string(get_age_category_name(static_cast<age_category>(this->get_base_value())));
	}

	virtual std::string get_value_name() const override
	{
		return "age";
	}
};

}
