#pragma once

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "character/character_modifier_type.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class character_attribute_modifier_condition final : public numerical_condition<character, read_only_context>
{
public:
	explicit character_attribute_modifier_condition(const gsml_operator condition_operator)
		: numerical_condition<character, read_only_context>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character_attribute_modifier";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "attribute") {
			this->attribute = character_attribute::get(value);
		} else if (key == "modifier_type") {
			this->modifier_type = magic_enum::enum_cast<character_modifier_type>(value).value();
		} else if (key == "modifier") {
			this->set_base_value(std::stoi(value));
			this->set_condition_operator(property.get_operator());
		} else {
			numerical_condition<character, read_only_context>::process_gsml_property(property);
		}
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return scope->get_game_data()->get_best_attribute_modifier(this->attribute, this->modifier_type.value());
	}

	virtual std::string get_value_name() const override
	{
		return std::format("{} ({} Bonus)", this->attribute->get_name(), get_character_modifier_type_name(this->modifier_type.value()));;
	}

private:
	const character_attribute *attribute = nullptr;
	std::optional<character_modifier_type> modifier_type;
};

}
