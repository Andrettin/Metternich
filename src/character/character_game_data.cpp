#include "metternich.h"

#include "character/character_game_data.h"

#include "character/character.h"
#include "character/character_type.h"
#include "character/trait.h"
#include "country/country.h"
#include "game/game.h"

namespace metternich {

character_game_data::character_game_data(const metternich::character *character) : character(character)
{
	connect(game::get(), &game::turn_changed, this, &character_game_data::age_changed);

	this->calculate_attributes();
}

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

int character_game_data::get_age() const
{
	const QDate birth_date = this->character->get_birth_date().date();
	const QDate current_date = game::get()->get_date().date();

	int age = current_date.year() - birth_date.year() - 1;

	const QDate current_birthday(current_date.year(), birth_date.month(), birth_date.day());
	if (current_date >= current_birthday) {
		++age;
	}

	return age;
}

void character_game_data::calculate_attributes()
{
	for (const trait *trait : this->character->get_traits()) {
		for (const auto &[attribute, bonus] : trait->get_attribute_bonuses()) {
			this->attribute_values[attribute] += bonus;
		}
	}

	emit attributes_changed();
}

int character_game_data::get_primary_attribute_value() const
{
	return this->get_attribute_value(this->character->get_type()->get_primary_attribute());
}

}
