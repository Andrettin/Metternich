#include "metternich.h"

#include "character/character_game_data.h"

#include "character/character.h"
#include "character/character_type.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "game/character_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/condition.h"
#include "script/modifier.h"
#include "script/scripted_character_modifier.h"
#include "spell/spell.h"
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
}

void character_game_data::on_game_started()
{
	for (const trait *trait : this->character->get_traits()) {
		this->add_trait(trait);
	}

	this->generate_missing_traits();

	this->check_portrait();
}

void character_game_data::do_turn()
{
	this->decrement_scripted_modifiers();
	this->do_events();
}

void character_game_data::do_events()
{
	const bool is_last_turn_of_year = (game::get()->get_date().date().month() + defines::get()->get_months_per_turn()) > 12;

	if (is_last_turn_of_year) {
		character_event::check_events_for_scope(this->character, event_trigger::yearly_pulse);
	}

	character_event::check_events_for_scope(this->character, event_trigger::quarterly_pulse);
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

	const character_type *character_type = this->character->get_type();
	const condition<metternich::character> *portrait_conditions = character_type->get_portrait_conditions(this->get_portrait());
	if (portrait_conditions == nullptr && character_type->get_portrait() != this->get_portrait()) {
		//portrait not available for the character type
		return false;
	}

	if (portrait_conditions != nullptr) {
		return portrait_conditions->check(this->character, read_only_context(this->character));
	}

	return true;
}

void character_game_data::check_portrait()
{
	if (this->is_current_portrait_valid()) {
		return;
	}

	const icon_map<std::unique_ptr<const condition<metternich::character>>> &conditional_portraits = this->character->get_type()->get_conditional_portraits();

	if (!conditional_portraits.empty()) {
		std::vector<const icon *> potential_portraits;
		const read_only_context ctx(this->character);

		for (const auto &[portrait, conditions] : conditional_portraits) {
			if (!conditions->check(this->character, ctx)) {
				continue;
			}

			potential_portraits.push_back(portrait);
		}

		//there must always be an available conditional portrait if conditional portraits are defined
		assert_throw(!potential_portraits.empty());

		this->portrait = vector::get_random(potential_portraits);
	} else {
		this->portrait = this->character->get_type()->get_portrait();
	}
}

void character_game_data::set_country(const metternich::country *country)
{
	if (country == this->get_country()) {
		return;
	}

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->remove_character(this->character);
	}

	this->country = country;

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->add_character(this->character);
	}

	if (game::get()->is_running()) {
		emit country_changed();
	}
}

void character_game_data::check_country()
{
	//check whether the character should change their country

	if (this->is_deployed()) {
		return;
	}

	const metternich::country *home_province_owner = this->character->get_home_province()->get_game_data()->get_owner();

	if (home_province_owner != this->get_country()) {
		//move the character to its home province's country, if it has nothing keeping it at a different country
		this->set_country(home_province_owner);
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

	if (trait->is_item()) {
		const std::vector<const metternich::trait *> current_items = this->get_traits_of_type(trait->get_type());

		assert_throw(current_items.size() <= 1);
		if (!current_items.empty() && current_items.front()->get_level() >= trait->get_level()) {
			//cannot receive an item trait if we already have an item trait of the same type with a greater or equal level
			return false;
		}
	}

	if (trait->get_spell() != nullptr && this->has_spell(trait->get_spell())) {
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

	if (trait->get_spell() != nullptr) {
		assert_throw(!this->has_spell(trait->get_spell()));
		this->add_item_spell(trait->get_spell());
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

	if (trait->get_military_unit_modifier() != nullptr && this->get_military_unit() != nullptr) {
		this->apply_military_unit_modifier(this->get_military_unit(), -1);
	}

	if (trait->get_spell() != nullptr) {
		assert_throw(this->has_item_spell(trait->get_spell()));
		this->remove_item_spell(trait->get_spell());
	}

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

const trait *character_game_data::generate_trait(const trait_type trait_type, const int max_level)
{
	std::vector<const trait *> potential_traits;
	int best_level = 0;

	const read_only_context ctx(this->character);

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

		if (trait->get_generation_conditions() != nullptr && !trait->get_generation_conditions()->check(this->character, ctx)) {
			continue;
		}

		if (trait->get_level() > max_level) {
			continue;
		}

		if (trait->get_level() > best_level) {
			potential_traits.clear();
			best_level = trait->get_level();
		} else if (trait->get_level() < best_level) {
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

void character_game_data::generate_missing_traits()
{
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
}

void character_game_data::sort_traits()
{
	std::sort(this->traits.begin(), this->traits.end(), [](const trait *lhs, const trait *rhs) {
		if (lhs->get_type() != rhs->get_type()) {
			return lhs->get_type() < rhs->get_type();
		}

		if (lhs->get_level() != rhs->get_level()) {
			return lhs->get_level() > rhs->get_level();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

int character_game_data::get_total_expertise_trait_level() const
{
	int level = 0;

	for (const trait *trait : this->get_traits()) {
		if (trait->get_type() != trait_type::expertise) {
			continue;
		}

		level += trait->get_level();
	}

	return level;
}

void character_game_data::gain_item(const trait *item)
{
	assert_throw(item != nullptr);
	assert_throw(item->is_item());

	//remove items of the same type but of a lower level, and grant to other characters
	const std::vector<const trait *> old_items = this->get_traits_of_type(item->get_type());

	for (const trait *old_item : old_items) {
		this->remove_trait(old_item);
	}

	this->add_trait(item);

	for (const trait *old_item : old_items) {
		//give the item to the country, which will reassign it to an appropriate character
		this->get_country()->get_game_data()->gain_item(old_item);
	}
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

bool character_game_data::is_deployable() const
{
	const character_type *character_type = this->character->get_type();

	if (character_type->get_military_unit_category() == military_unit_category::none) {
		return false;
	}

	if (!character_type->is_commander() && this->get_spells().empty()) {
		//non-commanders must know at least one spell in order to be deployable
		//the presumption being that non-commander character types which nevertheless have a military unit category will be support units, such as clerics and mages
		return false;
	}

	return true;
}

void character_game_data::deploy_to_province(const province *province)
{
	assert_throw(this->get_country() != nullptr);
	assert_throw(!this->is_deployed());
	assert_throw(this->is_deployable());

	const military_unit_type *military_unit_type = this->get_country()->get_game_data()->get_best_military_unit_category_type(this->character->get_type()->get_military_unit_category());

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

QString character_game_data::get_country_modifier_string(const unsigned indent) const
{
	if (this->character->get_type()->get_country_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->character->get_type()->get_country_modifier()->get_string(this->character->get_skill(), indent));
}

QString character_game_data::get_province_modifier_string(const unsigned indent) const
{
	if (this->character->get_type()->get_province_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->character->get_type()->get_province_modifier()->get_string(this->character->get_skill(), indent));
}

void character_game_data::apply_modifier(const modifier<const metternich::character> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	modifier->apply(this->character, multiplier);
}

void character_game_data::apply_country_modifier(const metternich::country *country, const int multiplier)
{
	if (this->character->get_type()->get_country_modifier() != nullptr) {
		this->character->get_type()->get_country_modifier()->apply(country, this->character->get_skill() * multiplier);
	}
}

void character_game_data::apply_province_modifier(const province *province, const int multiplier)
{
	if (this->character->get_type()->get_province_modifier() != nullptr) {
		this->character->get_type()->get_province_modifier()->apply(province, this->character->get_skill() * multiplier);
	}
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
	if (!vector::contains(spell->get_character_types(), this->character->get_type())) {
		return false;
	}

	if (this->has_learned_spell(spell)) {
		return false;
	}

	return true;
}

void character_game_data::learn_spell(const spell *spell)
{
	if (this->has_item_spell(spell)) {
		this->remove_item_spell(spell);

		const std::vector<const trait *> traits = this->get_traits();
		for (const trait *trait : traits) {
			if (trait->get_spell() == spell) {
				//if we are learning a spell that otherwise would be granted to us by an item, give the item to someone else instead
				assert_throw(trait->is_item());
				this->remove_trait(trait);
				this->get_country()->get_game_data()->gain_item(trait);
			}
		}
	}

	this->add_spell(spell);
}

}
