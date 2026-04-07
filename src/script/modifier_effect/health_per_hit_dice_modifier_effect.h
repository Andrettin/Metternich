#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class health_per_hit_dice_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit health_per_hit_dice_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "health_per_hit_dice";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const character *scope, const centesimal_int &multiplier) const override
	{
		co_await scope->get_game_data()->change_health_bonus_per_hit_dice((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Health per Hit Dice";
	}
};

}
