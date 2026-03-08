#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/skill.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class skill_condition final : public numerical_condition<character, read_only_context>
{
public:
	explicit skill_condition(const skill *skill, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character, read_only_context>(value, condition_operator), skill(skill)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "skill";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return scope->get_game_data()->get_effective_skill_value(this->skill);
	}

	virtual std::string get_value_name() const override
	{
		return this->skill->get_name();
	}

private:
	const metternich::skill *skill = nullptr;
};

}
