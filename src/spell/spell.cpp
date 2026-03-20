#include "metternich.h"

#include "spell/spell.h"

#include "character/character_class.h"
#include "database/defines.h"
#include "game/attack_result.h"
#include "spell/arcane_school.h"
#include "spell/spell_effect.h"
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

void spell::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "arcane_schools") {
		for (const std::string &value : values) {
			this->arcane_schools.push_back(arcane_school::get(value));
		}
	} else if (tag == "character_classes") {
		for (const std::string &value : values) {
			this->character_classes.push_back(character_class::get(value));
		}
	} else if (tag == "effects") {
		scope.for_each_element([&](const gsml_property &property) {
			auto effect = spell_effect::from_gsml_property(property);
			this->effects.push_back(std::move(effect));
		}, [&](const gsml_data &child_scope) {
			auto effect = spell_effect::from_gsml_scope(child_scope);
			this->effects.push_back(std::move(effect));
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void spell::check() const
{
	if (this->get_level() == 0) {
		throw std::runtime_error(std::format("Spell \"{}\" has no level.", this->get_identifier()));
	}

	assert_throw(this->get_target() != spell_target::none || this->get_battle_target() != spell_target::none);
	assert_throw(this->get_icon() != nullptr);
	assert_throw(!this->effects.empty() || this->get_battle_result() != attack_result::none);
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

}
