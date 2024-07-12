#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/gsml_data.h"
#include "spell/spell_effect.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"

namespace metternich {

class damage_spell_effect final : public spell_effect
{
	Q_OBJECT

	Q_PROPERTY(int value MEMBER value)

public:
	damage_spell_effect()
	{
	}

	explicit damage_spell_effect(const std::string &value)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "damage";
		return identifier;
	}

	int get_damage(const military_unit *caster) const
	{
		Q_UNUSED(caster);

		return this->value;
	}

	virtual void apply(military_unit *caster, military_unit *target) const override
	{
		target->receive_damage(this->get_damage(caster), 0);
	}

	virtual std::string get_string(const military_unit *caster) const override
	{
		return "Deal " + std::to_string(this->get_damage(caster)) + " Damage";
	}

private:
	int value = 0;
};

}
