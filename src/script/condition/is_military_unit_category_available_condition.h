#pragma once

#include "script/condition/condition.h"
#include "unit/military_unit_category.h"

namespace metternich {

class is_military_unit_category_available_condition final : public condition<country>
{
public:
	explicit is_military_unit_category_available_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->category = enum_converter<military_unit_category>::to_enum(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "is_military_unit_category_available";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_best_military_unit_category_type(this->category) != nullptr;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} available", get_military_unit_category_name(this->category));
	}

private:
	military_unit_category category = military_unit_category::none;
};

}
