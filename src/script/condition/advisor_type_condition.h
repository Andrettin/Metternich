#pragma once

#include "character/advisor_type.h"
#include "character/character.h"
#include "script/condition/condition.h"

namespace metternich {

class advisor_type_condition final : public condition<character>
{
public:
	explicit advisor_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->advisor_type = advisor_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "advisor_type";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_advisor_type() == this->advisor_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->advisor_type->get_name() + " advisor type";
	}

private:
	const metternich::advisor_type *advisor_type = nullptr;
};

}
