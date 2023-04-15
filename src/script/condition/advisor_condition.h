#pragma once

#include "character/character.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/condition.h"
#include "util/vector_util.h"

namespace metternich {

class advisor_condition final : public condition<country>
{
public:
	explicit advisor_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->advisor = character::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "advisor";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return vector::contains(scope->get_game_data()->get_advisors(), this->advisor);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->advisor->get_full_name() + " advisor";
	}

private:
	const character *advisor = nullptr;
};

}
