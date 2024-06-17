#pragma once

#include "country/country.h"
#include "country/subject_type.h"
#include "script/condition/condition.h"

namespace metternich {

class subject_type_condition final : public condition<country>
{
public:
	explicit subject_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->subject_type = subject_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "subject_type";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_subject_type() == this->subject_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} subject type", string::highlight(this->subject_type->get_name()));
	}

private:
	const metternich::subject_type *subject_type = nullptr;
};

}
