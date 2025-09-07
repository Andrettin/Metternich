#include "metternich.h"

#include "character/character_history.h"

#include "character/character.h"

namespace metternich {

character_history::character_history(const metternich::character *character) : character(character)
{
	this->level = character->get_level();
}

}
