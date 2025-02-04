#pragma once

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class character_attribute_condition final : public numerical_condition<character, read_only_context>
{
public:
	explicit character_attribute_condition(const character_attribute attribute, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character, read_only_context>(value, condition_operator), attribute(attribute)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character_attribute";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return scope->get_game_data()->get_attribute_value(this->attribute);
	}

	virtual std::string get_value_name() const override
	{
		return this->attribute->get_name();
	}

private:
	character_attribute attribute = character_attribute::none;
};

}
