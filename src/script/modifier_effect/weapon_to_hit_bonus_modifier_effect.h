#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "item/item_class.h"
#include "item/item_type.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/assert_util.h"

namespace metternich {

class weapon_to_hit_bonus_modifier_effect final : public modifier_effect<const character>
{
public:
	weapon_to_hit_bonus_modifier_effect() = default;

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "weapon_to_hit_bonus";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "weapon_type") {
			this->weapon_type = item_type::get(value);
		} else if (key == "weapon_class") {
			this->weapon_class = item_class::get(value);
		} else if (key == "bonus") {
			this->value = decimillesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		if (this->weapon_type != nullptr) {
			this->apply_to_weapon_types(scope, { this->weapon_type }, multiplier);
		} else if (this->weapon_class != nullptr) {
			this->apply_to_weapon_types(scope, this->weapon_class->get_item_types(), multiplier);
		} else {
			assert_throw(false);
		}
	}

	void apply_to_weapon_types(const character *scope, const std::vector<const item_type *> &weapon_types, const decimillesimal_int &multiplier) const
	{
		for (const item_type *weapon_type : weapon_types) {
			scope->get_game_data()->change_weapon_to_hit_bonus(weapon_type, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		if (this->weapon_type != nullptr) {
			return std::format("{} To Hit Bonus", this->weapon_type->get_name());
		} else if (this->weapon_class != nullptr) {
			return std::format("{} To Hit Bonus", this->weapon_class->get_name());
		} else {
			assert_throw(false);
		}

		return std::string();
	}

private:
	const metternich::item_type *weapon_type = nullptr;
	const metternich::item_class *weapon_class = nullptr;
};

}
