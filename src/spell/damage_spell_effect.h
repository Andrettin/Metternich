#pragma once

#include "character/attribute.h"
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

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "scaled") {
			const attribute attribute = enum_converter<metternich::attribute>::to_enum(scope.get_property_value("attribute"));
			const int value = std::stoi(scope.get_property_value("value"));
			this->scaled_values[attribute] += value;
		} else {
			spell_effect::process_gsml_scope(scope);
		}
	}

	int get_damage(const military_unit *caster) const
	{
		int damage = this->value;

		assert_throw(caster->get_character() != nullptr);

		for (const auto &[attribute, scaled_value] : this->scaled_values) {
			damage += scaled_value * caster->get_character()->get_game_data()->get_attribute_value(attribute);
		}

		return damage;
	}

	virtual void apply(military_unit *caster, military_unit *target) const override
	{
		target->receive_damage(this->get_damage(caster));
	}

	virtual std::string get_string(const military_unit *caster) const override
	{
		return "Deal " + std::to_string(this->get_damage(caster)) + " Damage";
	}

private:
	int value = 0;
	std::map<attribute, int> scaled_values;
};

}
