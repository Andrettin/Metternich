#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_stat.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class character_stat_condition final : public numerical_condition<character, read_only_context>
{
public:
	explicit character_stat_condition(const character_stat *stat, const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character, read_only_context>(value, condition_operator), stat(stat)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character_stat";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		return scope->get_game_data()->get_stat_value(this->stat);
	}

	virtual std::string get_value_name() const override
	{
		return this->stat->get_name();
	}

private:
	const character_stat *stat = nullptr;
};

}
