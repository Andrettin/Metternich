#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "species/species.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class species_armor_class_bonus_modifier_effect final : public modifier_effect<const character>
{
public:
	species_armor_class_bonus_modifier_effect() = default;

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "species_armor_class_bonus";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "species") {
			this->species = species::get(value);
		} else if (key == "bonus") {
			this->value = centesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		if (this->species != nullptr) {
			this->apply_to_species(scope, { this->species }, multiplier);
		} else {
			assert_throw(false);
		}
	}

	void apply_to_species(const character *scope, const std::vector<const species *> &species_list, const centesimal_int &multiplier) const
	{
		for (const metternich::species *species : species_list) {
			scope->get_game_data()->change_species_armor_class_bonus(species, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		if (this->species != nullptr) {
			return std::format("Armor Class Against {}", this->species->get_name());
		} else {
			assert_throw(false);
			return {};
		}
	}

private:
	const metternich::species *species = nullptr;
};

}
