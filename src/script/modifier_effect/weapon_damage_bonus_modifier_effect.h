#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "item/item_type.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class weapon_damage_bonus_modifier_effect final : public modifier_effect<const character>
{
public:
	weapon_damage_bonus_modifier_effect() = default;

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "weapon_damage_bonus";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "weapon_type") {
			this->weapon_type = item_type::get(value);
		} else if (key == "bonus") {
			this->value = decimillesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		assert_throw(this->weapon_type != nullptr);

		this->apply_to_weapon_types(scope, { this->weapon_type }, multiplier);
	}

	void apply_to_weapon_types(const character *scope, const std::vector<const item_type *> &weapon_types, const decimillesimal_int &multiplier) const
	{
		for (const item_type *weapon_type : weapon_types) {
			scope->get_game_data()->change_weapon_damage_bonus(weapon_type, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		assert_throw(this->weapon_type != nullptr);

		return std::format("{} Damage Bonus", this->weapon_type->get_name());
	}

private:
	const metternich::item_type *weapon_type = nullptr;
};

}
