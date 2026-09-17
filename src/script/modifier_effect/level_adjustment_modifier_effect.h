#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class level_adjustment_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit level_adjustment_modifier_effect(const std::string &value)
		: modifier_effect<const character>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "level_adjustment";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const character *scope, const decimillesimal_int &multiplier) const override
	{
		co_await scope->get_game_data()->change_level_adjustment((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Level Adjustment";
	}
};

}
