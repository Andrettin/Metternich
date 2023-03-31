#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/gsml_data.h"
#include "spell/spell_effect.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"

namespace metternich {

class healing_spell_effect final : public spell_effect
{
	Q_OBJECT

	Q_PROPERTY(int value MEMBER value)

public:
	healing_spell_effect()
	{
	}

	explicit healing_spell_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "healing";
		return identifier;
	}

	int get_healing(const military_unit *caster) const
	{
		Q_UNUSED(caster);

		return this->value;
	}

	virtual void apply(military_unit *caster, military_unit *target) const override
	{
		target->heal(this->get_healing(caster));
	}

	virtual std::string get_string(const military_unit *caster) const override
	{
		return "Heal " + std::to_string(this->get_healing(caster)) + " Hit Points";
	}

private:
	int value = 0;
};

}
