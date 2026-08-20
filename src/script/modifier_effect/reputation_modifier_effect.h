#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class reputation_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit reputation_modifier_effect(const std::string &value)
		: modifier_effect<const character>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "reputation";
		return identifier;
	}

	virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_reputation((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Reputation";
	}
};

}
