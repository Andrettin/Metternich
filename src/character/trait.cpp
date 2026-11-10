#include "metternich.h"

#include "character/trait.h"

#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_stat.h"
#include "character/trait_type.h"
#include "domain/office.h"
#include "script/condition/and_condition.h"
#include "script/modifier_effect/character_stat_modifier_effect.h"
#include "script/modifier_effect/skill_training_modifier_effect.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "species/taxon.h"
#include "ui/ui_defines.h"
#include "util/gender.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace metternich {

trait::trait(const std::string &identifier)
	: named_data_entry(identifier)
{
}

trait::~trait()
{
}

void trait::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "weight") {
		const int factor = std::stoi(value);
		this->weight_factor = std::make_unique<metternich::factor<character>>(factor);
	} else if (key == "scaling_stat") {
		this->scaling_stat = character_stat::get_stat(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void trait::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "variant_names") {
		character_class::process_variant_name_scope(this->variant_names, scope);
	} else if (tag == "types") {
		for (const std::string &value : values) {
			trait_type *trait_type = trait_type::get(value);
			trait_type->add_trait(this);
			this->add_type(trait_type);
		}
	} else if (tag == "opposite_traits") {
		for (const std::string &value : values) {
			const trait *opposite_trait = trait::get(value);
			this->opposite_traits.push_back(opposite_trait);
		}
	} else if (tag == "attribute_bonuses") {
		scope.to_map(this->attribute_bonuses);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "gain_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->gain_conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else if (tag == "per_mythic_tier_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const int tier_interval = std::stoi(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const character>>();
			modifier->process_gsml_data(child_scope);
			this->per_mythic_tier_modifiers[tier_interval] = std::move(modifier);
		});
	} else if (tag == "per_divine_rank_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const int rank_interval = std::stoi(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const character>>();
			modifier->process_gsml_data(child_scope);
			this->per_divine_rank_modifiers[rank_interval] = std::move(modifier);
		});
	} else if (tag == "office_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const office *office = office::get(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const domain>>();
			modifier->process_gsml_data(child_scope);
			this->office_modifiers[office] = std::move(modifier);
		});
	} else if (tag == "scaled_office_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const office *office = office::get(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const domain>>();
			modifier->process_gsml_data(child_scope);
			this->scaled_office_modifiers[office] = std::move(modifier);
		});
	} else if (tag == "military_unit_modifier") {
		auto modifier = std::make_unique<metternich::modifier<military_unit>>();
		modifier->process_gsml_data(scope);
		this->military_unit_modifier = std::move(modifier);
	} else if (tag == "weight_factor") {
		this->weight_factor = std::make_unique<factor<character>>();
		this->weight_factor->process_gsml_data(scope);
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void trait::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Character trait \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_types().empty()) {
		throw std::runtime_error(std::format("Character trait \"{}\" has no type.", this->get_identifier()));
	}

	if (this->get_scaling_stat() == nullptr && !this->scaled_office_modifiers.empty()) {
		throw std::runtime_error(std::format("Character trait \"{}\" has scaled modifiers, but no scaling stat.", this->get_identifier()));
	}

	if (this->get_opposite_opinion() != 0 && this->get_opposite_traits().empty()) {
		throw std::runtime_error(std::format("Character trait \"{}\" has an opposite opinion value, but no listed opposite traits.", this->get_identifier()));
	}

	for (const trait *opposite_trait : this->get_opposite_traits()) {
		if (opposite_trait == this) {
			throw std::runtime_error(std::format("Character trait \"{}\" has itself listed as an opposite trait.", this->get_identifier()));
		}

		if (!vector::contains(opposite_trait->get_opposite_traits(), this)) {
			throw std::runtime_error(std::format("Character trait \"{}\" has trait \"{}\" listed as an opposite trait, but the opposition is not mutual.", this->get_identifier(), opposite_trait->get_identifier()));
		}
	}

	named_data_entry::check();
}

const std::string &trait::get_name(const taxon_base *taxon, const gender gender) const
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

int trait::get_attribute_bonus(const character_attribute *attribute) const
{
	return archimedes::map::get_or_default(this->get_attribute_bonuses(), attribute);
}

std::string trait::get_modifier_string(const int multiplier, const bool single_line) const
{
	std::string str;

	if (this->get_modifier() != nullptr) {
		if (single_line) {
			str = this->get_modifier()->get_single_line_string(nullptr, multiplier);
		} else {
			str = this->get_modifier()->get_string(nullptr, multiplier);
		}
	}

	const std::string separator = single_line ? ", " : "\n";

	if (this->get_same_opinion() != 0) {
		if (!str.empty()) {
			str += separator;
		}

		str += std::format("Same Trait Opinion: {}", string::colored(number::to_signed_string(this->get_same_opinion()), this->get_same_opinion() >= 0 ? ui_defines::get()->get_green_text_color() : ui_defines::get()->get_red_text_color()));
	}

	if (this->get_opposite_opinion() != 0) {
		if (!str.empty()) {
			str += separator;
		}

		str += std::format("Opposite Trait Opinion: {}", string::colored(number::to_signed_string(this->get_opposite_opinion()), this->get_opposite_opinion() >= 0 ? ui_defines::get()->get_green_text_color() : ui_defines::get()->get_red_text_color()));
	}

	for (const auto &[tier_interval, modifier] : this->get_per_mythic_tier_modifiers()) {
		if (!str.empty()) {
			str += separator;
		}

		str += std::format("Every {} mythic tier{}:{}{}", tier_interval, tier_interval > 1 ? "s" : "", single_line ? " " : "\n", single_line ? modifier->get_single_line_string(nullptr, multiplier) : modifier->get_string(nullptr, multiplier, 1));
	}

	for (const auto &[rank_interval, modifier] : this->get_per_divine_rank_modifiers()) {
		if (!str.empty()) {
			str += separator;
		}

		str += std::format("Every {} divine rank{}:{}{}", rank_interval, rank_interval > 1 ? "s" : "", single_line ? " " : "\n", single_line ? modifier->get_single_line_string(nullptr, multiplier) : modifier->get_string(nullptr, multiplier, 1));
	}

	return str;
}

QString trait::get_modifier_qstring() const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier_string(1, false));
}

QString trait::get_military_unit_modifier_string() const
{
	if (this->get_military_unit_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_military_unit_modifier()->get_string(nullptr));
}

bool trait::enables_stat(const character_stat *stat) const
{
	if (this->get_modifier() == nullptr) {
		return false;
	}

	const skill *skill = dynamic_cast<const metternich::skill *>(stat);
	if (skill == nullptr) {
		return false;
	}

	for (const auto &modifier_effect : this->get_modifier()->get_modifier_effects()) {
		if (const skill_training_modifier_effect *skill_training_modifier_effect = dynamic_cast<const metternich::skill_training_modifier_effect *>(modifier_effect.get())) {
			if (skill_training_modifier_effect->get_skill() == skill) {
				return true;
			}
		}
	}

	return false;
}

int trait::get_stat_change(const character_stat *stat) const
{
	if (this->get_modifier() == nullptr) {
		return 0;
	}

	decimillesimal_int stat_change;

	for (const auto &modifier_effect : this->get_modifier()->get_modifier_effects()) {
		if (const character_stat_modifier_effect *character_stat_modifier_effect = dynamic_cast<const metternich::character_stat_modifier_effect *>(modifier_effect.get())) {
			if (character_stat_modifier_effect->get_stat() == stat) {
				stat_change += character_stat_modifier_effect->get_value();
			}
		}
	}

	return stat_change.to_int();
}

}

#include "moc_trait.cpp"
