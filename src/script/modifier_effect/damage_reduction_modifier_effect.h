#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/damage_reduction_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class damage_reduction_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit damage_reduction_modifier_effect(const damage_reduction_type *type, const std::string &value)
		: modifier_effect<const character>(value), type(type)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "damage_reduction";
		return identifier;
	}

	virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_damage_reduction(this->type, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return this->type->get_name();
	}

private:
	const damage_reduction_type *type = nullptr;
};

}
