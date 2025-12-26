#pragma once

#include "map/site.h"
#include "map/site_attribute.h"
#include "map/site_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class site_attribute_condition final : public numerical_condition<site, read_only_context>
{
public:
	explicit site_attribute_condition(const site_attribute *attribute, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<site, read_only_context>(value, condition_operator), attribute(attribute)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "site_attribute";
		return class_identifier;
	}

	virtual int get_scope_value(const site *scope) const override
	{
		return scope->get_game_data()->get_attribute_value(this->attribute);
	}

	virtual std::string get_value_name() const override
	{
		return this->attribute->get_name();
	}

private:
	const site_attribute *attribute = nullptr;
};

}
