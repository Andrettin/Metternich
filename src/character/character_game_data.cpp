#include "metternich.h"

#include "character/character_game_data.h"

#include "character/character.h"
#include "country/country.h"
#include "game/game.h"

namespace metternich {

void character_game_data::set_employer(const metternich::country *employer)
{
	if (employer == this->get_employer()) {
		return;
	}

	this->employer = employer;

	if (game::get()->is_running()) {
		emit employer_changed();
	}
}

}
