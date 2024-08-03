#pragma once

#include "character/advisor_category.h"
#include "character/advisor_type.h"
#include "character/character.h"
#include "script/condition/condition.h"

namespace metternich {

class advisor_category_condition final : public condition<character>
{
public:
	explicit advisor_category_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->advisor_category = enum_converter<metternich::advisor_category>::to_enum(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "advisor_category";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (scope->get_advisor_type() == nullptr) {
			return false;
		}

		return scope->get_advisor_type()->get_category() == this->advisor_category;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} advisor category", get_advisor_category_name(this->advisor_category));
	}

private:
	metternich::advisor_category advisor_category = advisor_category::none;
};

}
