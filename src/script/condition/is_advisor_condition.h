#pragma once

#include "character/character.h"
#include "character/character_role.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

class is_advisor_condition final : public condition<character>
{
public:
	explicit is_advisor_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "is_advisor";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return (scope->get_role() == character_role::advisor) == this->value;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return "Advisor";
		} else {
			return "Not an advisor";
		}
	}

private:
	bool value = false;
};

}
