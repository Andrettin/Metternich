#include "metternich.h"

#include "character/character_game_data.h"

#include "character/advisor_type.h"
#include "character/character.h"
#include "character/character_role.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "game/character_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "map/province.h"
#include "script/condition/condition.h"
#include "script/modifier.h"
#include "script/scripted_character_modifier.h"
#include "spell/spell.h"
#include "ui/portrait.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

character_game_data::character_game_data(const metternich::character *character)
	: character(character)
{
	connect(game::get(), &game::turn_changed, this, &character_game_data::age_changed);

	this->portrait = this->character->get_portrait();
}

void character_game_data::on_setup_finished()
{
	for (const trait *trait : this->character->get_traits()) {
		this->add_trait(trait);
	}

	this->check_portrait();
}

bool character_game_data::is_current_portrait_valid() const
{
	if (this->get_portrait() == nullptr) {
		return false;
	}

	if (this->get_portrait() == this->character->get_portrait()) {
		//this is the character's explicitly-defined portrait
		return true;
	}

	assert_throw(this->get_portrait()->get_character_conditions() != nullptr);

	if (!this->get_portrait()->get_character_conditions()->check(this->character, read_only_context(this->character))) {
		return false;
	}

	return true;
}

void character_game_data::check_portrait()
{
	if (this->character->get_role() != character_role::ruler && this->character->get_role() != character_role::advisor) {
		//only rulers and advisors need portraits
		return;
	}

	if (this->is_current_portrait_valid()) {
		return;
	}

	std::vector<const metternich::portrait *> potential_portraits;

	for (const metternich::portrait *portrait : portrait::get_character_portraits()) {
		assert_throw(portrait->is_character_portrait());
		assert_throw(portrait->get_character_conditions() != nullptr);

		if (!portrait->get_character_conditions()->check(this->character, read_only_context(this->character))) {
			continue;
		}

		potential_portraits.push_back(portrait);
	}

	//there must always be an available portrait for characters which need them
	if (potential_portraits.empty()) {
		throw std::runtime_error(std::format("No portrait is suitable for character \"{}\".", this->character->get_identifier()));
	}

	this->portrait = vector::get_random(potential_portraits);
}

void character_game_data::set_country(const metternich::country *country)
{
	if (country == this->get_country()) {
		return;
	}

	this->country = country;

	if (game::get()->is_running()) {
		emit country_changed();
	}
}

int character_game_data::get_age() const
{
	const QDate &birth_date = this->character->get_birth_date();
	const QDate &current_date = game::get()->get_date();

	int age = current_date.year() - birth_date.year() - 1;

	const QDate current_birthday(current_date.year(), birth_date.month(), birth_date.day());
	if (current_date >= current_birthday) {
		++age;
	}

	return age;
}

void character_game_data::set_dead(const bool dead)
{
	if (dead == this->is_dead()) {
		return;
	}

	this->dead = dead;

	if (game::get()->is_running()) {
		emit dead_changed();
	}
}

void character_game_data::die()
{
	this->set_country(nullptr);
	this->set_dead(true);
}

QVariantList character_game_data::get_traits_qvariant_list() const
{
	return container::to_qvariant_list(this->get_traits());
}

std::vector<const trait *> character_game_data::get_traits_of_type(const trait_type trait_type) const
{
	std::vector<const trait *> traits;

	for (const trait *trait : this->get_traits()) {
		if (trait->get_type() != trait_type) {
			continue;
		}

		traits.push_back(trait);
	}

	return traits;
}

bool character_game_data::can_have_trait(const trait *trait) const
{
	if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(this->character, read_only_context(this->character))) {
		return false;
	}

	return true;
}

bool character_game_data::has_trait(const trait *trait) const
{
	return vector::contains(this->get_traits(), trait);
}

void character_game_data::add_trait(const trait *trait)
{
	if (vector::contains(this->get_traits(), trait)) {
		log::log_error("Tried to add trait \"" + trait->get_identifier() + "\" to character \"" + this->character->get_identifier() + "\", but they already have the trait.");
		return;
	}

	const read_only_context ctx(this->character);

	if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(this->character, ctx)) {
		log::log_error("Tried to add trait \"" + trait->get_identifier() + "\" to character \"" + this->character->get_identifier() + "\", for which the trait's conditions are not fulfilled.");
		return;
	}

	this->traits.push_back(trait);

	if (trait->get_modifier() != nullptr) {
		this->apply_modifier(trait->get_modifier());
	}

	if (trait->get_military_unit_modifier() != nullptr && this->get_military_unit() != nullptr) {
		this->apply_military_unit_modifier(this->get_military_unit(), 1);
	}

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

void character_game_data::remove_trait(const trait *trait)
{
	//remove modifiers that this character is applying on other scopes so that we reapply them later, as the trait change can affect them
	std::erase(this->traits, trait);

	if (trait->get_modifier() != nullptr) {
		this->remove_modifier(trait->get_modifier());
	}

	if (trait->get_ruler_modifier() != nullptr && this->is_ruler()) {
		assert_throw(this->get_country() != nullptr);
		trait->get_ruler_modifier()->remove(this->get_country());
	}

	if (trait->get_military_unit_modifier() != nullptr && this->get_military_unit() != nullptr) {
		trait->get_military_unit_modifier()->remove(this->get_military_unit());
	}

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
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

QVariantList character_game_data::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

bool character_game_data::has_scripted_modifier(const scripted_character_modifier *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

void character_game_data::add_scripted_modifier(const scripted_character_modifier *modifier, const int duration)
{
	const read_only_context ctx(this->character);

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (modifier->get_modifier() != nullptr) {
		this->apply_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void character_game_data::remove_scripted_modifier(const scripted_character_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		this->remove_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void character_game_data::decrement_scripted_modifiers()
{
	std::vector<const scripted_character_modifier *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_character_modifier *modifier : modifiers_to_remove) {
		this->remove_scripted_modifier(modifier);
	}
}

bool character_game_data::is_ruler() const
{
	return this->get_country() != nullptr && this->get_country()->get_game_data()->get_ruler() == this->character;
}

bool character_game_data::is_deployable() const
{
	if (this->character->get_military_unit_category() == military_unit_category::none) {
		return false;
	}

	return true;
}

void character_game_data::deploy_to_province(const province *province)
{
	assert_throw(province != nullptr);
	assert_throw(this->get_country() != nullptr);
	assert_throw(!this->is_deployed());
	assert_throw(this->is_deployable());

	const military_unit_type *military_unit_type = this->get_country()->get_game_data()->get_best_military_unit_category_type(this->character->get_military_unit_category(), this->character->get_culture());

	auto military_unit = make_qunique<metternich::military_unit>(military_unit_type, this->character);

	assert_throw(military_unit->can_move_to(province));

	military_unit->set_province(province);
	this->military_unit = military_unit.get();

	this->get_country()->get_game_data()->add_military_unit(std::move(military_unit));
}

void character_game_data::undeploy()
{
	assert_throw(this->is_deployed());

	this->military_unit->disband(false);
}

void character_game_data::apply_modifier(const modifier<const metternich::character> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	modifier->apply(this->character, multiplier);
}

void character_game_data::apply_military_unit_modifier(metternich::military_unit *military_unit, const int multiplier)
{
	for (const trait *trait : this->get_traits()) {
		if (trait->get_military_unit_modifier() != nullptr) {
			trait->get_military_unit_modifier()->apply(military_unit, multiplier);
		}
	}
}

QVariantList character_game_data::get_spells_qvariant_list() const
{
	return container::to_qvariant_list(this->get_spells());
}

bool character_game_data::can_learn_spell(const spell *spell) const
{
	if (!spell->is_available_for_military_unit_category(this->character->get_military_unit_category())) {
		return false;
	}

	if (this->has_learned_spell(spell)) {
		return false;
	}

	return true;
}

void character_game_data::learn_spell(const spell *spell)
{
	this->add_spell(spell);
}

}
