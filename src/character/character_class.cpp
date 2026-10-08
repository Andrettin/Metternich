#include "metternich.h"

#include "character/character_class.h"

#include "character/character_attribute.h"
#include "character/character_class_type.h"
#include "character/character_defines.h"
#include "character/domain_skill.h"
#include "character/level_value_table.h"
#include "character/save_type.h"
#include "character/skill.h"
#include "character/skill_group.h"
#include "character/starting_age_category.h"
#include "character/trait_type.h"
#include "domain/government_type.h"
#include "infrastructure/holding_type.h"
#include "item/item_class.h"
#include "item/item_slot.h"
#include "item/item_type.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "species/species.h"
#include "species/taxon.h"
#include "spell/spell.h"
#include "ui/ui_defines.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/number_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

void character_class::process_variant_name_scope(std::map<const taxon_base *, std::map<gender, std::string>> &variant_names, const gsml_data &scope)
{
	scope.for_each_property([&variant_names](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (const species *species = species::try_get(key)) {
			variant_names[species][gender::none] = value;
		} else if (const taxon *taxon = taxon::try_get(key)) {
			variant_names[taxon][gender::none] = value;
		} else {
			const gender gender = magic_enum::enum_cast<archimedes::gender>(key).value();
			variant_names[nullptr][gender] = value;
		}
	});

	scope.for_each_child([&variant_names](const gsml_data &child_scope) {
		const std::string &child_tag = child_scope.get_tag();

		const taxon_base *taxon = species::try_get(child_tag);
		if (taxon == nullptr) {
			taxon = taxon::get(child_tag);
		}

		character_class::process_variant_name_scope(variant_names[taxon], child_scope);
	});
}

void character_class::process_variant_name_scope(std::map<gender, std::string> &variant_names, const gsml_data &scope)
{
	scope.for_each_property([&variant_names](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		const gender gender = magic_enum::enum_cast<archimedes::gender>(key).value();
		variant_names[gender] = value;
	});
}

character_class::character_class(const std::string &identifier)
	: named_data_entry(identifier), military_unit_category(military_unit_category::none)
{
}

character_class::~character_class()
{
}

void character_class::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "base_class") {
		character_class *base_class = character_class::get(value);
		this->base_class = base_class;
	} else if (key == "primary_attribute") {
		this->primary_attributes = { character_attribute::get(value) };
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void character_class::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "prerequisite_classes") {
		for (const std::string &value : values) {
			character_class *prerequisite_class = character_class::get(value);
			this->prerequisite_classes.push_back(prerequisite_class);
			prerequisite_class->advanced_classes.push_back(this);
		}
	} else if (tag == "primary_attributes") {
		for (const std::string &value : values) {
			this->primary_attributes.push_back(character_attribute::get(value));
		}
	} else if (tag == "variant_names") {
		character_class::process_variant_name_scope(this->variant_names, scope);
	} else if (tag == "save_bonus_tables") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->save_bonus_tables[save_type::get(key)] = level_value_table::get(value);
		});
	} else if (tag == "skill_bonus_tables") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->skill_bonus_tables[skill::get(key)] = level_value_table::get(value);
		});
	} else if (tag == "domain_skill_bonus_tables") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->domain_skill_bonus_tables[domain_skill::get(key)] = level_value_table::get(value);
		});
	} else if (tag == "trait_gain_tables") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->trait_gain_tables[trait_type::get(key)] = level_value_table::get(value);
		});
	} else if (tag == "exceptional_attributes") {
		for (const std::string &value : values) {
			this->exceptional_attributes.insert(character_attribute::get(value));
		}
	} else if (tag == "class_skills") {
		for (const std::string &value : values) {
			this->class_skills.insert(skill::get(value));
		}
	} else if (tag == "class_skill_groups") {
		for (const std::string &value : values) {
			this->class_skill_groups.insert(skill_group::get(value));
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
	} else if (tag == "allowed_government_types") {
		for (const std::string &value : values) {
			const government_type *government_type = government_type::get(value);
			this->allowed_government_types.push_back(government_type);
		}
	} else if (tag == "allowed_equipment_types") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const item_slot *item_slot = item_slot::get(child_tag);

			for (const std::string &value : child_scope.get_values()) {
				const item_type *item_type = item_type::get(value);
				this->allowed_equipment_types[item_slot].push_back(item_type);
			}
		});
	} else if (tag == "allowed_equipment_classes") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const item_slot *item_slot = item_slot::get(child_tag);

			for (const std::string &value : child_scope.get_values()) {
				const item_class *item_class = item_class::get(value);
				this->allowed_equipment_classes[item_slot].push_back(item_class);
			}
		});
	} else if (tag == "min_attribute_values") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->min_attribute_values[character_attribute::get(key)] = std::stoi(value);
		});
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "rank_levels") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->rank_levels[key] = std::stoi(value);
		});
	} else if (tag == "level_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level = std::stoi(child_tag);
			if (!this->level_modifiers.contains(level)) {
				this->level_modifiers[level] = std::make_unique<metternich::modifier<const character>>();
			}
			this->level_modifiers[level]->process_gsml_data(child_scope);
		});
	} else if (tag == "recurring_level_modifiers") {
		assert_throw(this->get_max_level() != 0);

		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level_interval = std::stoi(child_tag);
			for (int i = level_interval; i <= this->get_max_level(); i += level_interval) {
				if (this->get_min_level() != 0 && i < this->get_min_level()) {
					continue;
				}

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

		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const item_type *item_type = item_type::get(key);
			const int quantity = std::stoi(value);

			for (int i = 0; i < quantity; ++i) {
				this->starting_items.push_back(item_type);
			}
		});
	} else if (tag == "starting_spells") {
		for (const std::string &value : values) {
			this->starting_spells.push_back(spell::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character_class::check() const
{
	if (this->get_min_level() > 0 && this->get_prerequisite_classes().empty()) {
		throw std::runtime_error(std::format("Character class \"{}\" has a minimum level, but has no prerequisite classes.", this->get_identifier()));
	}
	if (this->get_min_level() == 0 && !this->get_prerequisite_classes().empty()) {
		throw std::runtime_error(std::format("Character class \"{}\" has no minimum level, but has prerequisite classes.", this->get_identifier()));
	}

	switch (this->get_type()) {
		case character_class_type::base_class:
			if (!this->get_prerequisite_classes().empty()) {
				throw std::runtime_error(std::format("Character class \"{}\" is a base class, but has a prerequisite class.", this->get_identifier()));
			}
			if (this->get_min_level() > 0) {
				throw std::runtime_error(std::format("Character class \"{}\" is a base class, but has a minimum level.", this->get_identifier()));
			}
			break;
		case character_class_type::prestige_class:
			if (this->get_prerequisite_classes().empty()) {
				throw std::runtime_error(std::format("Character class \"{}\" is a prestige class, but has no prerequisite class.", this->get_identifier()));
			}
			if (this->get_min_level() == 0) {
				throw std::runtime_error(std::format("Character class \"{}\" is a prestige class, but has no minimum level.", this->get_identifier()));
			}
			break;
		case character_class_type::epic_class:
			if (this->get_prerequisite_classes().empty()) {
				throw std::runtime_error(std::format("Character class \"{}\" is an epic class, but has no prerequisite class.", this->get_identifier()));
			}
			if (this->get_min_level() <= 20) {
				throw std::runtime_error(std::format("Character class \"{}\" has is an epic class, but does not have a minimum level beyond 20.", this->get_identifier()));
			}
			break;
		default:
			break;
	}

	for (const character_class *prerequisite_class : this->get_prerequisite_classes()) {
		const character_class_type prerequisite_class_type = prerequisite_class->get_type();

		switch (this->get_type()) {
			case character_class_type::subclass:
				if (prerequisite_class_type != character_class_type::base_class) {
					throw std::runtime_error(std::format("Character class \"{}\" is a subclass, but its prerequisite class \"{}\" is not a base class.", this->get_identifier(), prerequisite_class->get_identifier()));
				}
				break;
			case character_class_type::prestige_class:
				if (prerequisite_class_type != character_class_type::base_class && prerequisite_class_type != character_class_type::subclass) {
					throw std::runtime_error(std::format("Character class \"{}\" is a prestige class, but its prerequisite class \"{}\" is neither a base class nor a subclass.", this->get_identifier(), prerequisite_class->get_identifier()));
				}
				break;
			case character_class_type::epic_class:
				if (prerequisite_class_type != character_class_type::base_class && prerequisite_class_type != character_class_type::subclass && prerequisite_class_type != character_class_type::prestige_class) {
					throw std::runtime_error(std::format("Character class \"{}\" is an epic class, but its prerequisite class \"{}\" is neither a base class, nor a subclass, nor a prestige class.", this->get_identifier(), prerequisite_class->get_identifier()));
				}
				break;
			default:
				break;
		}

		if (prerequisite_class->get_min_level() > this->get_min_level()) {
			throw std::runtime_error(std::format("Character class \"{}\" has prerequisite class \"{}\", but the latter has a higher minimum level.", this->get_identifier(), prerequisite_class->get_identifier()));
		}
	}

	if (this->get_primary_attributes().empty()) {
		throw std::runtime_error(std::format("Character class \"{}\" has no primary attributes.", this->get_identifier()));
	}

	if (this->get_max_level() == 0) {
		throw std::runtime_error(std::format("Character class \"{}\" has no max level.", this->get_identifier()));
	}

	if (this->get_starting_age_category() == starting_age_category::none) {
		throw std::runtime_error(std::format("Character class \"{}\" has no starting age category.", this->get_identifier()));
	}

	if (this->get_experience_table() == nullptr) {
		throw std::runtime_error(std::format("Character class \"{}\" has no experience table.", this->get_identifier()));
	}

	if (this->get_health_bonus_table() == nullptr) {
		throw std::runtime_error(std::format("Character class \"{}\" has no health bonus table.", this->get_identifier()));
	}

	if (this->get_reputation_bonus_table() == nullptr) {
		throw std::runtime_error(std::format("Character class \"{}\" has no reputation bonus table.", this->get_identifier()));
	}

	if (this->get_to_hit_bonus_table() == nullptr) {
		throw std::runtime_error(std::format("Character class \"{}\" has no to hit bonus table.", this->get_identifier()));
	}

	for (const save_type *save_type : save_type::get_all()) {
		const level_value_table *save_bonus_table = this->get_save_bonus_table(save_type);
		if (save_bonus_table == nullptr && save_type->get_base_save_type() == nullptr) {
			throw std::runtime_error(std::format("Character class \"{}\" has no save bonus table for base save type \"{}\".", this->get_identifier(), save_type->get_identifier()));
		}
	}

	if (this->get_class_skills().empty() && this->get_class_skill_groups().empty()) {
		throw std::runtime_error(std::format("Character class \"{}\" has neither class skills nor class skill groups.", this->get_identifier()));
	}

	for (const species *species : this->allowed_species) {
		if (species->get_character_class_level_limit(this) == 0) {
			throw std::runtime_error(std::format("Species \"{}\" is allowed for character class \"{}\", but has no level limit for it.", species->get_identifier(), this->get_identifier()));
		}
	}
}

metternich::military_unit_category character_class::get_military_unit_category() const
{
	if (this->military_unit_category != military_unit_category::none) {
		return this->military_unit_category;
	}

	if (this->get_base_class() != nullptr) {
		return this->get_base_class()->get_military_unit_category();
	}

	return military_unit_category::none;
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

const std::string &character_class::get_name(const taxon_base *taxon, const gender gender) const
{
	auto taxon_find_iterator = this->variant_names.find(taxon);
	if (taxon_find_iterator == this->variant_names.end()) {
		if (taxon != nullptr && taxon->get_supertaxon() != nullptr) {
			return this->get_name(taxon->get_supertaxon(), gender);
		}

		taxon_find_iterator = this->variant_names.find(nullptr);
	}

	if (taxon_find_iterator != this->variant_names.end()) {
		auto gender_find_iterator = taxon_find_iterator->second.find(gender);
		if (gender_find_iterator == taxon_find_iterator->second.end()) {
			gender_find_iterator = taxon_find_iterator->second.find(gender::none);
		}

		if (gender_find_iterator != taxon_find_iterator->second.end()) {
			return gender_find_iterator->second;
		}
	}

	return named_data_entry::get_name();
}

const level_value_table *character_class::get_experience_table() const
{
	if (this->experience_table != nullptr) {
		return this->experience_table;
	}

	if (this->get_base_class() != nullptr) {
		return this->get_base_class()->get_experience_table();
	}

	return character_defines::get()->get_default_experience_table();
}

bool character_class::has_class_skill(const skill *skill) const
{
	for (const skill_group *skill_group : skill->get_groups()) {
		if (this->get_class_skill_groups().contains(skill_group)) {
			return true;
		}
	}

	return this->get_class_skills().contains(skill);
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

bool character_class::is_government_type_allowed(const government_type *government_type) const
{
	if (this->allowed_government_types.empty()) {
		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->is_government_type_allowed(government_type);
		}
	}

	return vector::contains(this->allowed_government_types, government_type);
}

void character_class::add_allowed_government_type(const government_type *government_type)
{
	this->allowed_government_types.push_back(government_type);
}

bool character_class::is_equipment_type_allowed(const item_type *equipment_type) const
{
	const item_slot *item_slot = equipment_type->get_slot();

	const auto type_slot_find_iterator = this->allowed_equipment_types.find(item_slot);
	const auto class_slot_find_iterator = this->allowed_equipment_classes.find(item_slot);
	if (type_slot_find_iterator != this->allowed_equipment_types.end() || class_slot_find_iterator != this->allowed_equipment_classes.end()) {
		//if there is specifically designated allowed equipment for this item slot, only allow the equipment type if it matches the allowed equipment types or classes
		if (type_slot_find_iterator != this->allowed_equipment_types.end() && vector::contains(type_slot_find_iterator->second, equipment_type)) {
			return true;
		} else if (class_slot_find_iterator != this->allowed_equipment_classes.end() && vector::contains(class_slot_find_iterator->second, equipment_type->get_item_class())) {
			return true;
		} else {
			return false;
		}
	}

	if (this->get_base_class() != nullptr) {
		return this->get_base_class()->is_equipment_type_allowed(equipment_type);
	}

	//if there is no specifically designated allowed equipment for this item slot, then the equipment type is considered allowed by default
	return true;
}

std::string character_class::get_level_modifier_string(const int level, const metternich::character *character) const
{
	std::string str;

	const std::variant<int, dice> &health_bonus = this->get_health_bonus_table()->get_value_variant_for_level(level);
	if (std::holds_alternative<int>(health_bonus)) {
		const int health_bonus_int = std::get<int>(health_bonus);
		if (health_bonus_int != 0) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Health: {}", string::colored(number::to_signed_string(health_bonus_int), ui_defines::get()->get_green_text_color()));
		}
	} else if (std::holds_alternative<dice>(health_bonus)) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("Health: {}", string::colored("+" + std::get<dice>(health_bonus).to_display_string(), ui_defines::get()->get_green_text_color()));
	}

	const int to_hit_bonus = this->get_to_hit_bonus_table()->get_value_for_level(level);
	if (to_hit_bonus != 0) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("To Hit Bonus: {}", string::colored(number::to_signed_string(to_hit_bonus), ui_defines::get()->get_green_text_color()));
	}

	for (const save_type *save_type : save_type::get_all()) {
		const level_value_table *save_bonus_table = this->get_save_bonus_table(save_type);

		if (save_bonus_table == nullptr) {
			continue;
		}

		const int save_bonus = save_bonus_table->get_value_for_level(level);

		if (save_bonus != 0) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("{}: {}", save_type->get_name(), string::colored(number::to_signed_string(save_bonus), ui_defines::get()->get_green_text_color()));
		}
	}

	for (const skill *skill : skill::get_all()) {
		const level_value_table *skill_bonus_table = this->get_skill_bonus_table(skill);
		if (skill_bonus_table == nullptr) {
			continue;
		}
		const int skill_bonus = skill_bonus_table->get_value_for_level(level);

		if (skill_bonus != 0) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("{}: {}", skill->get_name(), string::colored(number::to_signed_string(skill_bonus), ui_defines::get()->get_green_text_color()));
		}
	}

	for (const domain_skill *domain_skill : domain_skill::get_all()) {
		const level_value_table *domain_skill_bonus_table = this->get_domain_skill_bonus_table(domain_skill);
		if (domain_skill_bonus_table == nullptr) {
			continue;
		}
		const int domain_skill_bonus = domain_skill_bonus_table->get_value_for_level(level);

		if (domain_skill_bonus != 0) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("{}: {}", domain_skill->get_name(), string::colored(number::to_signed_string(domain_skill_bonus), ui_defines::get()->get_green_text_color()));
		}
	}

	for (const trait_type *trait_type : trait_type::get_all()) {
		const level_value_table *trait_gain_table = this->get_trait_gain_table(trait_type);
		if (trait_gain_table == nullptr) {
			continue;
		}
		const int trait_gain_count = trait_gain_table->get_value_for_level(level);

		if (trait_gain_count != 0) {
			if (!str.empty()) {
				str += "\n";
			}

			if (trait_gain_count == 1) {
				str += std::format("Gain Trait of Type: {}", trait_type->get_name());
			} else {
				str += std::format("Gain {} Traits of Type: {}", trait_gain_count, trait_type->get_name());
			}
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
