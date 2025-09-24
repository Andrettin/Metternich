#include "metternich.h"

#include "character/character_reference.h"

#include "character/character.h"
#include "game/game.h"
#include "util/assert_util.h"

namespace metternich {

character_reference::character_reference(metternich::character *character) : character(character)
{
	assert_throw(character->is_temporary());
}

character_reference::~character_reference()
{
	game::get()->remove_generated_character(this->character);
}

}
