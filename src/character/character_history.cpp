#include "metternich.h"

#include "character/character_history.h"

#include "character/character.h"
#include "character/character_class.h"
#include "util/assert_util.h"

namespace metternich {

character_history::character_history(const metternich::character *character) : character(character)
{
	this->level = character->get_level();
}

void character_history::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "rank") {
		assert_throw(this->character->get_character_class() != nullptr);
		this->level = this->character->get_character_class()->get_rank_level(value);
	} else {
		data_entry_history::process_gsml_property(property);
	}
}

}
