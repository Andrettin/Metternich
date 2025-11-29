#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_attribute.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class domain_attribute_condition final : public numerical_condition<domain, read_only_context>
{
public:
	explicit domain_attribute_condition(const domain_attribute *attribute, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<domain, read_only_context>(value, condition_operator), attribute(attribute)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "domain_attribute";
		return class_identifier;
	}

	virtual int get_scope_value(const domain *scope) const override
	{
		return scope->get_game_data()->get_attribute_value(this->attribute);
	}

	virtual std::string get_value_name() const override
	{
		return this->attribute->get_name();
	}

private:
	const domain_attribute *attribute = nullptr;
};

}
