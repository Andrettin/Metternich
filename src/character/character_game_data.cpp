#include "metternich.h"

#include "character/character_game_data.h"

#include "character/attribute.h"
#include "character/character.h"
#include "character/character_type.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "country/country.h"
#include "game/game.h"
#include "script/modifier.h"
#include "util/container_util.h"
#include "util/vector_random_util.h"

namespace metternich {

character_game_data::character_game_data(const metternich::character *character) : character(character)
{
	connect(game::get(), &game::turn_changed, this, &character_game_data::age_changed);
}

void character_game_data::on_game_started()
{
	for (const trait *trait : this->character->get_traits()) {
		this->add_trait(trait);
	}

	std::set<trait_type> trait_types;

	for (const trait *trait : this->get_traits()) {
		trait_types.insert(trait->get_type());
	}

	static const std::vector<trait_type> required_trait_types = { trait_type::education, trait_type::background, trait_type::personality };

	for (const trait_type trait_type : required_trait_types) {
		if (!trait_types.contains(trait_type)) {
			this->generate_trait(trait_type);
		}
	}
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

QVariantList character_game_data::get_traits_qvariant_list() const
{
	return container::to_qvariant_list(this->get_traits());
}

void character_game_data::add_trait(const trait *trait)
{
	this->traits.push_back(trait);

	if (trait->get_modifier() != nullptr) {
		trait->get_modifier()->apply(this->character);
	}

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

void character_game_data::generate_trait(const trait_type trait_type)
{
	std::vector<const trait *> potential_traits;

	for (const trait *trait : trait::get_all()) {
		if (trait->get_type() != trait_type) {
			continue;
		}

		if (!trait->is_available_for_character_type(this->character->get_type())) {
			continue;
		}

		potential_traits.push_back(trait);
	}

	if (potential_traits.empty()) {
		return;
	}

	this->add_trait(vector::get_random(potential_traits));
}

int character_game_data::get_attribute_value(const attribute attribute) const
{
	const int value = this->get_unclamped_attribute_value(attribute);

	if (value >= 0) {
		return value;
	}

	if (attribute == this->character->get_type()->get_primary_attribute()) {
		//the primary attribute must always be at least 1, so that the character isn't completely useless
		return 1;
	}

	return 0;
}

void character_game_data::set_attribute_value(const attribute attribute, const int value)
{
	if (value == this->get_unclamped_attribute_value(attribute)) {
		return;
	}

	if (value == 0) {
		this->attribute_values.erase(attribute);
	} else {
		this->attribute_values[attribute] = value;
	}

	if (game::get()->is_running()) {
		emit attributes_changed();
	}
}

int character_game_data::get_primary_attribute_value() const
{
	return this->get_attribute_value(this->character->get_type()->get_primary_attribute());
}

int character_game_data::get_diplomacy() const
{
	return this->get_attribute_value(attribute::diplomacy);
}

int character_game_data::get_martial() const
{
	return this->get_attribute_value(attribute::martial);
}

int character_game_data::get_stewardship() const
{
	return this->get_attribute_value(attribute::stewardship);
}

int character_game_data::get_intrigue() const
{
	return this->get_attribute_value(attribute::intrigue);
}

int character_game_data::get_learning() const
{
	return this->get_attribute_value(attribute::learning);
}

}
