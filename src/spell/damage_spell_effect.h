#pragma once

#include "spell/spell_effect.h"
#include "unit/military_unit.h"

namespace metternich {

class damage_spell_effect final : public spell_effect
{
	Q_OBJECT

	Q_PROPERTY(int damage MEMBER damage)

public:
	damage_spell_effect()
	{
	}

	explicit damage_spell_effect(const std::string &value)
	{
		this->damage = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "damage";
		return identifier;
	}

	virtual void apply(military_unit *caster, military_unit *target) const override
	{
		Q_UNUSED(caster);

		target->receive_damage(this->damage);
	}

	virtual std::string get_string() const override
	{
		return "Deal " + std::to_string(this->damage) + " Damage";
	}

private:
	int damage = 0;
};

}
