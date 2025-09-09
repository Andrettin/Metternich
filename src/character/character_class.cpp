#include "metternich.h"

#include "character/character_class.h"

#include "character/character_attribute.h"
#include "character/starting_age_category.h"
#include "database/defines.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "ui/portrait.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"

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

	if (tag == "min_attribute_values") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->min_attribute_values[character_attribute::get(key)] = std::stoi(value);
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
			auto modifier = std::make_unique<metternich::modifier<const character>>();
			modifier->process_gsml_data(child_scope);
			this->level_modifiers[level] = std::move(modifier);
		});
	} else if (tag == "level_effects") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level = std::stoi(child_tag);
			auto effect_list = std::make_unique<metternich::effect_list<const character>>();
			effect_list->process_gsml_data(child_scope);
			this->level_effects[level] = std::move(effect_list);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character_class::check() const
{
	if (this->get_attribute() == nullptr) {
		throw std::runtime_error(std::format("Character type \"{}\" has no attribute.", this->get_identifier()));
	}

	if (this->get_max_level() == 0) {
		throw std::runtime_error(std::format("Character class \"{}\" has no max level.", this->get_identifier()));
	}

	if (this->get_hit_dice().is_null()) {
		throw std::runtime_error(std::format("Character class \"{}\" has null hit dice.", this->get_identifier()));
	}

	if (this->get_hit_dice().get_count() != 1) {
		throw std::runtime_error(std::format("Character class \"{}\" has hit dice with a dice count different than 1.", this->get_identifier()));
	}

	if (this->get_starting_age_category() == starting_age_category::none) {
		throw std::runtime_error(std::format("Character type \"{}\" has no starting age category.", this->get_identifier()));
	}
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
	std::string str = std::format("Hit Points: +{}", this->get_hit_dice().to_string());

	const modifier<const metternich::character> *level_modifier = this->get_level_modifier(level);
	if (level_modifier != nullptr) {
		str += "\n" + level_modifier->get_string(character);
	}

	const effect_list<const metternich::character> *level_effects = this->get_level_effects(level);
	if (level_effects != nullptr) {
		str += "\n" + level_effects->get_effects_string(character, read_only_context(character));
	}

	return str;
}

}
