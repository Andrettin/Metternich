#include "metternich.h"

#include "character/character_history.h"

#include "character/character.h"
#include "character/character_class.h"
#include "character/trait.h"
#include "util/assert_util.h"

namespace metternich {

character_history::character_history(const metternich::character *character) : character(character)
{
	this->level = character->get_level();
	this->traits = character->get_traits();
}

void character_history::process_gsml_property(const gsml_property &property, const QDate &date)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "rank") {
		assert_throw(this->character->get_character_class() != nullptr);
		this->level = this->character->get_character_class()->get_rank_level(value);
	} else if (key == "trait") {
		const trait *trait = trait::get(value);
		this->traits.push_back(trait);
	} else {
		data_entry_history::process_gsml_property(property, date);
	}
}

void character_history::set_spouse(const metternich::character *spouse)
{
	if (spouse == this->get_spouse()) {
		return;
	}

	const metternich::character *old_spouse = this->get_spouse();

	this->spouse = spouse;

	if (old_spouse != nullptr) {
		old_spouse->get_history()->set_spouse(nullptr);
	}

	if (spouse != nullptr) {
		spouse->get_history()->set_spouse(this->character);
	}
}

}
