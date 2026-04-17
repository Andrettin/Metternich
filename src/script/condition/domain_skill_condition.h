#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/domain_skill.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class domain_skill_condition final : public numerical_condition<character, read_only_context>
{
public:
	explicit domain_skill_condition(const domain_skill *domain_skill, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character, read_only_context>(value, condition_operator), domain_skill(domain_skill)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "domain_skill";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return scope->get_game_data()->get_domain_skill_value(this->domain_skill);
	}

	virtual std::string get_value_name() const override
	{
		return this->domain_skill->get_name();
	}

private:
	const metternich::domain_skill *domain_skill = nullptr;
};

}
