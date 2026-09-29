#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class general_opinion_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit general_opinion_modifier_effect(const decimillesimal_int &value)
		: modifier_effect<const character>(value)
	{
	}

	explicit general_opinion_modifier_effect(const std::string &value)
		: modifier_effect<const character>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "general_opinion";
		return identifier;
	}

	virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_general_opinion_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "General Opinion";
	}
};

}
