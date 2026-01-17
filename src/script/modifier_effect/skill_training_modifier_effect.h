#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/skill.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class skill_training_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit skill_training_modifier_effect(const std::string &value)
	{
		this->skill = skill::get(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "skill_training";
		return identifier;
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		assert_throw(this->skill != nullptr);

		scope->get_game_data()->change_skill_training(this->skill, multiplier.to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		assert_throw(this->skill != nullptr);

		return std::format("{} Training", this->skill->get_name());
	}

	virtual std::string get_string(const character *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return this->get_base_string(scope);
	}

private:
	const metternich::skill *skill = nullptr;
};

}
