#pragma once

#include "character/character.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

class ruler_condition final : public condition<country>
{
public:
	explicit ruler_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<metternich::country>(condition_operator)
	{
		this->ruler = character::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "ruler";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_ruler() == this->ruler;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} ruler", this->ruler->get_full_name());
	}

private:
	const character *ruler = nullptr;
};

}
