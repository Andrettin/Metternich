#pragma once

#include "character/character.h"
#include "character/character_attribute.h"
#include "script/condition/condition.h"

namespace metternich {

class primary_attribute_condition final : public condition<character>
{
public:
	explicit primary_attribute_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->attribute = enum_converter<character_attribute>::to_enum(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "primary_attribute";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_primary_attribute() == this->attribute;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} primary attribute", get_character_attribute_name(this->attribute));
	}

private:
	character_attribute attribute = character_attribute::none;
};

}
