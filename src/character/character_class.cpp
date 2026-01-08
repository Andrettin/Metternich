#include "metternich.h"

#include "character/character_class.h"

#include "character/character_attribute.h"
#include "character/level_bonus_table.h"
#include "character/saving_throw_type.h"
#include "character/skill.h"
#include "character/skill_group.h"
#include "character/starting_age_category.h"
#include "database/defines.h"
#include "infrastructure/holding_type.h"
#include "item/item_type.h"
#include "script/condition/and_condition.h"
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
	} else if (tag == "class_skills") {
		for (const std::string &value : values) {
			this->class_skills.insert(skill::get(value));
		}
	} else if (tag == "class_skill_groups") {
		for (const std::string &value : values) {
			this->class_skill_groups.push_back(skill_group::get(value));
		}
	} else if (tag == "allowed_species") {
		for (const std::string &value : values) {
			this->allowed_species.push_back(species::get(value));
		}
	} else if (tag == "allowed_holding_types") {
		for (const std::string &value : values) {
			this->allowed_holding_types.push_back(holding_type::get(value));
		}
	} else if (tag == "favored_holding_types") {
		for (const std::string &value : values) {
			const holding_type *holding_type = holding_type::get(value);
			this->favored_holding_types.push_back(holding_type);
			this->allowed_holding_types.push_back(holding_type);
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
	} else if (tag == "starting_items") {
		for (const std::string &value : values) {
			this->starting_items.push_back(item_type::get(value));
		}

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const item_type *item_type = item_type::get(key);
			const int quantity = std::stoi(value);

			for (int i = 0; i < quantity; ++i) {
				this->starting_items.push_back(item_type);
			}
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character_class::initialize()
{
	for (const skill_group *skill_group : this->class_skill_groups) {
		for (const skill *group_skill : skill_group->get_skills()) {
			this->class_skills.insert(group_skill);
		}
	}

	named_data_entry::initialize();
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

	for (const saving_throw_type *saving_throw_type : saving_throw_type::get_all()) {
		const level_bonus_table *saving_throw_bonus_table = this->get_saving_throw_bonus_table(saving_throw_type);
		if (saving_throw_bonus_table == nullptr) {
			throw std::runtime_error(std::format("Character class \"{}\" has no saving throw bonus table for saving throw type \"{}\".", this->get_identifier(), saving_throw_type->get_identifier()));
		}
	}

	if (this->get_class_skills().empty()) {
		throw std::runtime_error(std::format("Character class \"{}\" has no class skills.", this->get_identifier()));
	}

	for (const species *species : this->allowed_species) {
		if (species->get_character_class_level_limit(this) == 0) {
			throw std::runtime_error(std::format("Species \"{}\" is allowed for character class \"{}\", but has no level limit for it.", species->get_identifier(), this->get_identifier()));
		}
	}
}

metternich::starting_age_category character_class::get_starting_age_category() const
{
	if (this->starting_age_category != starting_age_category::none) {
		return this->starting_age_category;
	}

	if (this->get_base_class() != nullptr) {
		return this->get_base_class()->get_starting_age_category();
	}

	return starting_age_category::none;
}

bool character_class::is_allowed_for_species(const species *species) const
{
	if (this->allowed_species.empty()) {
		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->is_allowed_for_species(species);
		}

		//FIXME: remove this once all character classes have a list of allowed species
		return true;
	}

	return vector::contains(this->allowed_species, species);
}

void character_class::add_allowed_species(const species *species)
{
	this->allowed_species.push_back(species);
}

bool character_class::is_holding_type_allowed(const holding_type *holding_type) const
{
	if (this->allowed_holding_types.empty()) {
		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->is_holding_type_allowed(holding_type);
		}
	}

	return vector::contains(this->allowed_holding_types, holding_type);
}

bool character_class::is_holding_type_favored(const holding_type *holding_type) const
{
	if (this->favored_holding_types.empty() && this->allowed_holding_types.empty()) {
		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->is_holding_type_favored(holding_type);
		}
	}

	return vector::contains(this->favored_holding_types, holding_type);
}

int64_t character_class::get_experience_for_level(const int level) const
{
	const auto find_iterator = this->experience_per_level.find(level);
	if (find_iterator != this->experience_per_level.end()) {
		return find_iterator->second;
	}

	if (this->get_base_class() != nullptr) {
		return this->get_base_class()->get_experience_for_level(level);
	}

	return defines::get()->get_experience_for_level(level);
}

std::string character_class::get_level_modifier_string(const int level, const metternich::character *character) const
{
	std::string str;

	const int to_hit_bonus = this->get_to_hit_bonus_table()->get_bonus_per_level(level);
	if (to_hit_bonus != 0) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("To Hit Bonus: +{}", to_hit_bonus);
	}

	for (const saving_throw_type *saving_throw_type : saving_throw_type::get_all()) {
		const level_bonus_table *saving_throw_bonus_table = this->get_saving_throw_bonus_table(saving_throw_type);
		const int saving_throw_bonus = saving_throw_bonus_table->get_bonus_per_level(level);
		if (saving_throw_bonus != 0) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("{}: +{}", saving_throw_type->get_name(), saving_throw_bonus);
		}
	}

	const modifier<const metternich::character> *level_modifier = this->get_level_modifier(level);
	if (level_modifier != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += level_modifier->get_string(character);
	}

	return str;
}

}
