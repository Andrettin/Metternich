#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class trait_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit trait_modifier_effect(const std::string &value)
	{
		this->trait = trait::get(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "trait";
		return identifier;
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_trait_count(this->trait, multiplier.to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Trait";
	}

	virtual std::string get_string(const character *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(ignore_decimals);

		return std::format("{} {}: {}", multiplier > 0 ? "Gain" : "Lose", this->get_base_string(scope), this->trait->get_name());
	}

private:
	const metternich::trait *trait = nullptr;
};

}
