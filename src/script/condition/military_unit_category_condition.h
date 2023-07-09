#pragma once

#include "script/condition/condition.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "util/string_util.h"

namespace metternich {

class military_unit_category_condition final : public condition<military_unit>
{
public:
	explicit military_unit_category_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<military_unit>(condition_operator)
	{
		this->military_unit_category = enum_converter<metternich::military_unit_category>::to_enum(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "military_unit_category";
		return class_identifier;
	}

	virtual bool check_assignment(const military_unit *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_category() == this->military_unit_category;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(get_military_unit_category_name(this->military_unit_category));
	}

private:
	metternich::military_unit_category military_unit_category = metternich::military_unit_category::none;
};

}
