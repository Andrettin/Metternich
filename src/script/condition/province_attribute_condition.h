#pragma once

#include "map/province.h"
#include "map/province_attribute.h"
#include "map/province_game_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class province_attribute_condition final : public numerical_condition<province, read_only_context>
{
public:
	explicit province_attribute_condition(const province_attribute *attribute, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<province, read_only_context>(value, condition_operator), attribute(attribute)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "province_attribute";
		return class_identifier;
	}

	virtual int get_scope_value(const province *scope) const override
	{
		return scope->get_game_data()->get_attribute_value(this->attribute);
	}

	virtual std::string get_value_name() const override
	{
		return this->attribute->get_name();
	}

private:
	const province_attribute *attribute = nullptr;
};

}
