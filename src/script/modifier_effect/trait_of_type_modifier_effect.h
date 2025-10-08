#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class trait_of_type_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit trait_of_type_modifier_effect(const std::string &value)
	{
		this->trait_type = trait_type::get(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "trait_of_type";
		return identifier;
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		if (multiplier > 0) {
			scope->get_game_data()->add_trait_of_type(this->trait_type);
		} else if (multiplier < 0) {
			scope->get_game_data()->remove_trait_of_type(this->trait_type);
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Trait of Type";
	}

	virtual std::string get_string(const character *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(ignore_decimals);

		return std::format("{} {}: {}", multiplier > 0 ? "Gain" : "Lose", this->get_base_string(scope), this->trait_type->get_name());
	}

private:
	const metternich::trait_type *trait_type = nullptr;
};

}
