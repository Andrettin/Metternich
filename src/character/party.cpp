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
		throw std::runtime_error(std::format("Tried to remove character \"{}\" from a party, but it is not a part of it.", character->get_game_data()->get_titled_name()));
	}

	std::erase(this->characters, character);
}

void party::gain_experience(const int64_t experience)
{
	assert_throw(experience >= 0);

	if (experience > 0) {
		const int64_t experience_per_character = experience / static_cast<int64_t>(this->get_characters().size());
		for (const character *character : this->get_characters()) {
			character->get_game_data()->change_experience(experience_per_character);
		}
	}
}

const character *party::get_best_skill_character(const skill *skill) const
{
	int best_skill_value = 0;
	const character *best_skill_character = nullptr;

	for (const character *character : this->get_characters()) {
		if (best_skill_character == nullptr || (character->get_game_data()->is_skill_trained(skill) && character->get_game_data()->get_skill_value(skill) > best_skill_value)) {
			best_skill_character = character;
			best_skill_value = character->get_game_data()->is_skill_trained(skill) ? character->get_game_data()->get_skill_value(skill) : 0;
		}
	}

	return best_skill_character;
}

}
