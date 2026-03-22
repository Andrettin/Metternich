#pragma once

#include "character/character.h"
#include "domain/office.h"
#include "script/condition/condition.h"

namespace metternich {

class office_condition final : public condition<character>
{
public:
	explicit office_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->office = office::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "office";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_office() == this->office;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{}", this->office->get_name());
	}

private:
	const metternich::office *office = nullptr;
};

}
