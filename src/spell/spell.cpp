#include "metternich.h"

#include "spell/spell.h"

#include "character/character_class.h"
#include "character/character_defines.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "economy/commodity.h"
#include "game/attack_result.h"
#include "game/battle.h"
#include "item/item_type.h"
#include "religion/divine_domain.h"
#include "script/context.h"
#include "script/effect/effect_list.h"
#include "spell/arcane_school.h"
#include "spell/spell_target.h"
#include "spell/spell_type.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/number_util.h"
#include "util/string_conversion_util.h"
#include "util/vector_util.h"

namespace metternich {

spell::spell(const std::string &identifier)
	: named_data_entry(identifier)
{
}

spell::~spell()
{
}

void spell::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "price") {
		this->price = defines::get()->get_wealth_commodity()->string_to_value(value);
	} else if (key == "range") {
		if (value == "touch") {
			this->range = character_defines::get()->get_minimum_character_range();
		} else {
			this->range = string::to_length(value);
		}
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void spell::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "arcane_schools") {
		for (const std::string &value : values) {
			arcane_school *arcane_school = arcane_school::get(value);
			this->arcane_schools.push_back(arcane_school);
			arcane_school->add_spell(this);
		}
	} else if (tag == "divine_domains") {
		for (const std::string &value : values) {
			divine_domain *divine_domain = divine_domain::get(value);
			this->divine_domains.push_back(divine_domain);
			divine_domain->add_spell(this);
		}
	} else if (tag == "character_classes") {
		for (const std::string &value : values) {
			this->character_classes.push_back(character_class::get(value));
		}
	} else if (tag == "character_class_levels") {
		scope.for_each_property([this](const gsml_property &property) {
			const character_class *character_class = character_class::get(property.get_key());
			const int level = std::stoi(property.get_value());
			this->character_class_levels[character_class] = level;
			this->character_classes.push_back(character_class);
		});
	} else if (tag == "material_components") {
		for (const std::string &value : values) {
			this->material_components.push_back(item_type::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const item_type *item_type = item_type::get(property.get_key());
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->material_components.push_back(item_type);
			}
		});
	} else if (tag == "target_character_effects") {
		auto effects = std::make_unique<effect_list<const character>>();
		effects->process_gsml_data(scope);
		this->target_character_effects = std::move(effects);
	} else if (tag == "target_military_unit_effects") {
		auto effects = std::make_unique<effect_list<military_unit>>();
		effects->process_gsml_data(scope);
		this->target_military_unit_effects = std::move(effects);
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void spell::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_spell(this);
	}

	named_data_entry::initialize();
}

void spell::check() const
{
	if (this->get_type() == spell_type::none) {
		throw std::runtime_error(std::format("Spell \"{}\" has no type.", this->get_identifier()));
	}

	if (this->get_level() == -1 && (does_spell_type_cost_mana(this->get_type()) || !this->get_divine_domains().empty())) {
		throw std::runtime_error(std::format("Spell \"{}\" has no level, and it is either of a mana-costing type or can be granted by deities. In both of those cases, the spell needs a level.", this->get_identifier()));
	}

	assert_throw(this->get_icon() != nullptr);

	if (this->get_price() == 0 && this->is_item_learnable()) {
		throw std::runtime_error(std::format("Spell \"{}\" can be learned from items, but has no price to add to the item price.", this->get_identifier()));
	}

	assert_throw(this->get_target() != spell_target::none);
	assert_throw(this->get_target_character_effects() != nullptr || this->get_target_military_unit_effects() != nullptr || this->get_battle_result() != attack_result::none || this->is_weapon_attack());

	switch (this->get_target()) {
		case spell_target::ally:
		case spell_target::ally_character:
		case spell_target::enemy:
		case spell_target::enemy_character:
			if (this->get_range() == 0) {
				throw std::runtime_error(std::format("Spell \"{}\" has an ally or enemy target, but no range.", this->get_identifier()));
			}
			break;
		default:
			break;
	}

	if ((this->get_target() == spell_target::ally || this->get_target() == spell_target::enemy) && this->get_battle_result() == attack_result::none && this->get_target_military_unit_effects() == nullptr && !this->is_weapon_attack()) {
		throw std::runtime_error(std::format("Spell \"{}\" has a military unit target, but no target military unit effects.", this->get_identifier()));
	}

	if ((this->get_target() == spell_target::ally_character || this->get_target() == spell_target::enemy_character) && this->get_target_character_effects() == nullptr && !this->is_weapon_attack()) {
		throw std::runtime_error(std::format("Spell \"{}\" has a character target, but no target character effects.", this->get_identifier()));
	}

	for (const character_class *character_class : this->get_character_classes()) {
		if (character_class->is_divine_spellcaster() && this->get_divine_domains().empty()) {
			log::log_error(std::format("Spell \"{}\" can be cast by a divine spellcasting class, but has no divine domains.", this->get_identifier()));
			break;
		}
	}
}

int spell::get_mana_cost(const character_class *character_class) const
{
	if (this->mana_cost != 0) {
		return this->mana_cost;
	}

	if (!does_spell_type_cost_mana(this->get_type())) {
		return 0;
	}

	assert_throw(this->get_level() != -1);

	return character_defines::get()->get_mana_cost_for_spell_level(this->get_level_for_character_class(character_class));
}

int spell::get_battle_range() const
{
	if (this->get_range() == 0) {
		return 0;
	}

	return battle::length_to_battle_range(this->get_range()).to_int();
}

bool spell::is_item_learnable() const
{
	return is_spell_type_item_learnable(this->get_type());
}

bool spell::is_available_for_character_class(const character_class *character_class) const
{
	return vector::contains(this->get_character_classes(), character_class);
}

int spell::get_level_for_character_class(const character_class *character_class) const
{
	const auto find_iterator = this->character_class_levels.find(character_class);
	if (find_iterator != this->character_class_levels.end()) {
		return find_iterator->second;
	}

	return this->get_level();
}

bool spell::is_combat_spell() const
{
	return this->get_target() != spell_target::none;
}

bool spell::is_battle_spell() const
{
	return this->get_target() != spell_target::none;
}

QString spell::get_combat_effects_string(const metternich::character *caster) const
{
	read_only_context ctx;
	ctx.source_scope = caster;

	std::string str = std::format("Target: {}, Range: {}, Target Effect: {}", get_spell_target_name(this->get_target()), this->get_range(), this->get_target_character_effects()->get_effects_single_line_string(nullptr, ctx));

	return QString::fromStdString(str);
}

QString spell::get_battle_effects_string(const metternich::character *caster) const
{
	read_only_context ctx;
	ctx.source_scope = caster;

	std::string effects_str;

	if (this->is_weapon_attack()) {
		effects_str = "Weapon Attack";

		if (this->get_to_hit_modifier() != 0 || this->get_damage_modifier() != 0 || this->get_weapon_damage_dice_multiplier() != 0) {
			std::string weapon_modifier_str;

			if (this->get_to_hit_modifier() != 0) {
				weapon_modifier_str += std::format("{} To Hit", number::to_signed_string(this->get_to_hit_modifier()));
			}
			if (this->get_weapon_damage_dice_multiplier() != 0) {
				if (!weapon_modifier_str.empty()) {
					weapon_modifier_str += ", ";
				}

				weapon_modifier_str += std::format("x{} Weapon Damage", this->get_weapon_damage_dice_multiplier());
			}
			if (this->get_damage_modifier() != 0) {
				if (!weapon_modifier_str.empty()) {
					weapon_modifier_str += ", ";
				}

				weapon_modifier_str += std::format("{} Damage", number::to_signed_string(this->get_damage_modifier()));
			}

			effects_str += std::format(" ({})", weapon_modifier_str);
		}
	}

	if (this->get_battle_result() != attack_result::none) {
		if (!effects_str.empty()) {
			effects_str += ", ";
		}

		effects_str = get_attack_result_name(this->get_battle_result());
	} else if (this->get_target_military_unit_effects() != nullptr) {
		if (!effects_str.empty()) {
			effects_str += ", ";
		}

		effects_str = this->get_target_military_unit_effects()->get_effects_single_line_string(nullptr, ctx);
	} else if (this->get_target_character_effects() != nullptr) {
		if (!effects_str.empty()) {
			effects_str += ", ";
		}

		effects_str = this->get_target_character_effects()->get_effects_single_line_string(nullptr, ctx);
	}

	std::string str = std::format("Target: {}, Range: {}, Effect: {}", get_spell_target_name(this->get_target()), this->get_battle_range(), effects_str);

	return QString::fromStdString(str);
}

}
