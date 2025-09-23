#include "metternich.h"

#include "character/party.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace metternich {

party::party(const std::vector<const character *> &characters)
	: characters(characters)
{
	assert_throw(!this->get_characters().empty());

	this->domain = this->get_characters().at(0)->get_game_data()->get_domain();
}

party::~party()
{
}

QVariantList party::get_characters_qvariant_list() const
{
	return container::to_qvariant_list(this->get_characters());
}

void party::remove_character(const character *character)
{
	if (!vector::contains(this->get_characters(), character)) {
		throw std::runtime_error(std::format("Tried to remove character \"{}\" from a party, but it is not a part of it.", character->get_full_name()));
	}

	std::erase(this->characters, character);
}

}
