#include "metternich.h"

#include "character/character_game_data.h"

#include "character/attribute.h"
#include "character/character.h"
#include "character/character_type.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/landed_title.h"
#include "game/game.h"
#include "script/condition/condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

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

	static const std::vector<trait_type> required_trait_types = { trait_type::background, trait_type::personality };

	for (const trait_type trait_type : required_trait_types) {
		if (!trait_types.contains(trait_type)) {
			this->generate_trait(trait_type, 1);
		}
	}

	this->generate_expertise_traits();
}

void character_game_data::set_employer(const metternich::country *employer)
{
	if (employer == this->get_employer()) {
		return;
	}

	assert_throw(this->get_office() == nullptr);

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

bool character_game_data::has_trait(const trait *trait) const
{
	return vector::contains(this->get_traits(), trait);
}

void character_game_data::add_trait(const trait *trait)
{
	const read_only_context ctx = read_only_context::from_scope(this->character);

	if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(this->character, ctx)) {
		throw std::runtime_error("Tried to add trait \"" + trait->get_identifier() + "\" to character \"" + this->character->get_identifier() + "\", for which the trait's conditions are not fulfilled.");
	}

	//remove modifiers that this character is applying on other scopes so that we reapply them later, as the trait change can affect them
	if (this->is_ruler()) {
		this->apply_country_modifier(this->get_employer(), -1);
	}

	this->traits.push_back(trait);

	if (trait->get_modifier() != nullptr) {
		trait->get_modifier()->apply(this->character);
	}

	const modifier<const metternich::character> *character_type_modifier = trait->get_character_type_modifier(this->character->get_type());
	if (character_type_modifier != nullptr) {
		character_type_modifier->apply(this->character);
	}

	//reapply modifiers that this character is applying on other scopes
	if (this->is_ruler()) {
		this->apply_country_modifier(this->get_employer(), 1);
	}

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

const trait *character_game_data::generate_trait(const trait_type trait_type, const int max_level)
{
	std::vector<const trait *> potential_traits;

	const read_only_context ctx = read_only_context::from_scope(this->character);

	for (const trait *trait : trait::get_all()) {
		if (trait->get_type() != trait_type) {
			continue;
		}

		if (this->has_trait(trait)) {
			continue;
		}

		if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(this->character, ctx)) {
			continue;
		}

		if (trait->get_level() > max_level) {
			continue;
		}

		potential_traits.push_back(trait);
	}

	if (potential_traits.empty()) {
		return nullptr;
	}

	const trait *trait = vector::get_random(potential_traits);
	this->add_trait(trait);
	return trait;
}

void character_game_data::generate_expertise_traits()
{
	const int initial_trait_level = this->get_total_trait_level();

	for (int i = initial_trait_level; i < this->character->get_level();) {
		const trait *trait = this->generate_trait(trait_type::expertise, this->character->get_level() - i);
		if (trait == nullptr) {
			return;
		}

		i += trait->get_level();
	}
}

void character_game_data::sort_traits()
{
	std::sort(this->traits.begin(), this->traits.end(), [](const trait *lhs, const trait *rhs) {
		if (lhs->get_type() != rhs->get_type()) {
			return lhs->get_type() < rhs->get_type();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

int character_game_data::get_total_trait_level() const
{
	int level = 0;

	for (const trait *trait : this->get_traits()) {
		level += trait->get_level();
	}

	return level;
}

int character_game_data::get_attribute_value(const attribute attribute) const
{
	const int value = this->get_unclamped_attribute_value(attribute);

	if (value > 0) {
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

int character_game_data::get_skill() const
{
	return this->get_attribute_value(attribute::skill);
}

int character_game_data::get_prowess() const
{
	return this->get_attribute_value(attribute::prowess);
}

int character_game_data::get_vitality() const
{
	return this->get_attribute_value(attribute::vitality);
}

QVariantList character_game_data::get_landed_titles_qvariant_list() const
{
	return container::to_qvariant_list(this->get_landed_titles());
}

bool character_game_data::is_ruler() const
{
	return this->get_employer() != nullptr && this->get_employer()->get_game_data()->get_ruler() == this->character;
}

void character_game_data::set_office(const metternich::office *office)
{
	if (office == this->get_office()) {
		return;
	}

	this->office = office;

	if (game::get()->is_running()) {
		emit office_changed();
	}
}

QString character_game_data::get_country_modifier_string() const
{
	if (this->character->get_type()->get_country_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->character->get_type()->get_country_modifier()->get_string(this->get_primary_attribute_value()));
}

QString character_game_data::get_province_modifier_string() const
{
	if (this->character->get_type()->get_province_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->character->get_type()->get_province_modifier()->get_string(this->get_primary_attribute_value()));
}

void character_game_data::apply_country_modifier(const country *country, const int multiplier)
{
	if (this->character->get_type()->get_country_modifier() != nullptr) {
		this->character->get_type()->get_country_modifier()->apply(country, this->get_primary_attribute_value() * multiplier);
	}
}

void character_game_data::apply_province_modifier(const province *province, const int multiplier)
{
	if (this->character->get_type()->get_province_modifier() != nullptr) {
		this->character->get_type()->get_province_modifier()->apply(province, this->get_primary_attribute_value() * multiplier);
	}
}

}
