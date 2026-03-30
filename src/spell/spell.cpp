#include "metternich.h"

#include "spell/spell.h"

#include "character/character_class.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "game/attack_result.h"
#include "religion/divine_domain.h"
#include "script/effect/effect_list.h"
#include "spell/arcane_school.h"
#include "spell/spell_target.h"
#include "util/assert_util.h"
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
	} else {
		data_entry::process_gsml_property(property);
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
	} else if (tag == "effects") {
		auto effects = std::make_unique<effect_list<const character>>();
		effects->process_gsml_data(scope);
		this->effects = std::move(effects);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void spell::check() const
{
	if (this->get_level() == 0) {
		throw std::runtime_error(std::format("Spell \"{}\" has no level.", this->get_identifier()));
	}

	assert_throw(this->get_icon() != nullptr);

	if (this->get_price() == 0) {
		throw std::runtime_error(std::format("Spell \"{}\" has no price.", this->get_identifier()));
	}

	assert_throw(this->get_target() != spell_target::none || this->get_battle_target() != spell_target::none);
	assert_throw(this->get_effects() != nullptr || this->get_battle_result() != attack_result::none);
}

int spell::get_mana_cost() const
{
	if (this->mana_cost != 0) {
		return this->mana_cost;
	}

	return defines::get()->get_mana_cost_for_spell_level(this->get_level());
}

bool spell::is_available_for_character_class(const character_class *character_class) const
{
	return vector::contains(this->get_character_classes(), character_class);
}

bool spell::is_combat_spell() const
{
	return this->get_target() != spell_target::none;
}

bool spell::is_battle_spell() const
{
	return this->get_battle_target() != spell_target::none;
}

QString spell::get_battle_effects_string() const
{
	std::string str = std::format("Target: {}, Range: {}, Effect: {}", get_spell_target_name(this->get_battle_target()), this->get_battle_range(), get_attack_result_name(this->get_battle_result()));

	return QString::fromStdString(str);
}

}
