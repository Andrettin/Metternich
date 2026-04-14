#pragma once

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "domain/domain.h"
#include "domain/domain_attribute.h"
#include "domain/domain_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

template <typename scope_type>
class attribute_check_chance_condition final : public numerical_condition<scope_type, read_only_context>
{
public:
	using attribute_type = std::conditional_t<std::is_same_v<scope_type, domain>, domain_attribute, character_attribute>;

	explicit attribute_check_chance_condition(const gsml_operator condition_operator)
		: numerical_condition<scope_type, read_only_context>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "attribute_check_chance";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "attribute") {
			this->attribute = attribute_type::get(value);
		} else if (key == "chance") {
			this->set_base_value(std::stoi(value));
			this->set_condition_operator(property.get_operator());
		} else {
			numerical_condition<scope_type, read_only_context>::process_gsml_property(property);
		}
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		return scope->get_game_data()->get_attribute_check_chance(this->attribute, 0);
	}

	virtual std::string get_base_value_string() const override
	{
		return numerical_condition<scope_type, read_only_context>::get_base_value_string() + "%";
	}

	virtual std::string get_value_name() const override
	{
		return std::format("{} Check Chance", this->attribute->get_name());
	}

private:
	const attribute_type *attribute = nullptr;
};

}
