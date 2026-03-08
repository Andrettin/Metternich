#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "religion/deity.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class divine_rank_condition final : public numerical_condition<character, read_only_context>
{
public:
	explicit divine_rank_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<character, read_only_context>(value, condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "divine_rank";
		return class_identifier;
	}

	virtual int get_scope_value(const character *scope) const override
	{
		if (!scope->get_game_data()->is_deity()) {
			return 0;
		}

		return scope->get_deity()->get_divine_level();
	}

	virtual std::string get_value_name() const override
	{
		return "Divine Rank";
	}
};

}
