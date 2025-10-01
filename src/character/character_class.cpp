#include "metternich.h"

#include "character/character_class.h"

#include "character/character_attribute.h"
#include "character/level_bonus_table.h"
#include "character/saving_throw_type.h"
#include "character/starting_age_category.h"
#include "database/defines.h"
#include "item/item_type.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "species/species.h"
#include "ui/portrait.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

character_class::character_class(const std::string &identifier)
	: named_data_entry(identifier), military_unit_category(military_unit_category::none)
{
}

character_class::~character_class()
{
}

void character_class::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "saving_throw_bonus_tables") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->saving_throw_bonus_tables[saving_throw_type::get(key)] = level_bonus_table::get(value);
		});
	} else if (tag == "allowed_species") {
		for (const std::string &value : values) {
			this->allowed_species.push_back(species::get(value));
		}
	} else if (tag == "min_attribute_values") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->min_attribute_values[character_attribute::get(key)] = std::stoi(value);
		});
	} else if (tag == "rank_levels") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->rank_levels[key] = std::stoi(value);
		});
	} else if (tag == "experience_per_level") {
		scope.for_each_property([&](const gsml_property &property) {
			const int level = std::stoi(property.get_key());
			const int64_t experience = std::stoll(property.get_value());

			this->experience_per_level[level] = experience;
		});
	} else if (tag == "level_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level = std::stoi(child_tag);
			if (!this->level_modifiers.contains(level)) {
				this->level_modifiers[level] = std::make_unique<metternich::modifier<const character>>();
			}
			this->level_modifiers[level]->process_gsml_data(child_scope);
		});
	} else if (tag == "recurring_level_modifiers") {
		assert_throw(this->get_max_level() != 0);

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level_interval = std::stoi(child_tag);
			for (int i = level_interval; i <= this->get_max_level(); i += level_interval) {
				if (!this->level_modifiers.contains(i)) {
					this->level_modifiers[i] = std::make_unique<metternich::modifier<const character>>();
				}
				this->level_modifiers[i]->process_gsml_data(child_scope);
			}
		});
	} else if (tag == "level_effects") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level = std::stoi(child_tag);
			if (!this->level_effects.contains(level)) {
				this->level_effects[level] = std::make_unique<metternich::effect_list<const character>>();
			}
			this->level_effects[level]->process_gsml_data(child_scope);
		});
	} else if (tag == "recurring_level_effects") {
		assert_throw(this->get_max_level() != 0);

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level_interval = std::stoi(child_tag);
			for (int i = level_interval; i <= this->get_max_level(); i += level_interval) {
				if (!this->level_effects.contains(i)) {
					this->level_effects[i] = std::make_unique<metternich::effect_list<const character>>();
				}
				this->level_effects[i]->process_gsml_data(child_scope);
			}
		});
	} else if (tag == "starting_items") {
		for (const std::string &value : values) {
			this->starting_items.push_back(item_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character_class::check() const
{
	if (this->get_attribute() == nullptr) {
		throw std::runtime_error(std::format("Character class \"{}\" has no attribute.", this->get_identifier()));
	}

	if (this->get_max_level() == 0) {
		throw std::runtime_error(std::format("Character class \"{}\" has no max level.", this->get_identifier()));
	}

	if (this->get_starting_age_category() == starting_age_category::none) {
		throw std::runtime_error(std::format("Character class \"{}\" has no starting age category.", this->get_identifier()));
	}

	if (this->get_to_hit_bonus_table() == nullptr) {
		throw std::runtime_error(std::format("Character class \"{}\" has no to hit bonus table.", this->get_identifier()));
	}

	if (this->get_saving_throw_bonus_tables().empty()) {
		throw std::runtime_error(std::format("Character class \"{}\" has no saving throw bonus tables.", this->get_identifier()));
	}

	for (const species *species : this->allowed_species) {
		if (species->get_character_class_level_limit(this) == 0) {
			throw std::runtime_error(std::format("Species \"{}\" is allowed for character class \"{}\", but has no level limit for it.", species->get_identifier(), this->get_identifier()));
		}
	}
}

bool character_class::is_allowed_for_species(const species *species) const
{
	//FIXME: remove this once all character classes have a list of allowed species
	if (this->allowed_species.empty()) {
		return true;
	}

	return vector::contains(this->allowed_species, species);
}

void character_class::add_allowed_species(const species *species)
{
	this->allowed_species.push_back(species);
}

int64_t character_class::get_experience_for_level(const int level) const
{
	const auto find_iterator = this->experience_per_level.find(level);
	if (find_iterator != this->experience_per_level.end()) {
		return find_iterator->second;
	}

	return defines::get()->get_experience_for_level(level);
}

std::string character_class::get_level_effects_string(const int level, const metternich::character *character) const
{
	std::string str;

	const int to_hit_bonus = this->get_to_hit_bonus_table()->get_bonus_per_level(level);
	if (to_hit_bonus != 0) {
		str += std::format("\nTo Hit Bonus: +{}", to_hit_bonus);
	}

	for (const auto &[saving_throw_type, saving_throw_bonus_table] : this->get_saving_throw_bonus_tables()) {
		const int saving_throw_bonus = saving_throw_bonus_table->get_bonus_per_level(level);
		if (saving_throw_bonus != 0) {
			str += std::format("\n{}: +{}", saving_throw_type->get_name(), saving_throw_bonus);
		}
	}

	const modifier<const metternich::character> *level_modifier = this->get_level_modifier(level);
	if (level_modifier != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += level_modifier->get_string(character);
	}

	const effect_list<const metternich::character> *level_effects = this->get_level_effects(level);
	if (level_effects != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += level_effects->get_effects_string(character, read_only_context(character));
	}

	return str;
}

}
