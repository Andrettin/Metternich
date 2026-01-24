#pragma once

#include "character/character.h"
#include "domain/country_government.h"
#include "domain/domain.h"
#include "script/condition/condition.h"

namespace metternich {

class ruler_condition final : public condition<domain>
{
public:
	explicit ruler_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<metternich::domain>(condition_operator)
	{
		this->ruler = character::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "ruler";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_government()->get_ruler() == this->ruler;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} ruler", this->ruler->get_game_data()->get_full_name());
	}

private:
	const character *ruler = nullptr;
};

}
