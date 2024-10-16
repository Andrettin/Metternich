#include "metternich.h"

#include "character/character_game_data.h"

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_role.h"
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
#include "script/condition/condition.h"
#include "script/effect/effect_list.h"
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
#include "util/string_util.h"
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

	static constexpr std::array generated_trait_types{ trait_type::background, trait_type::personality, trait_type::expertise };

	bool success = true;
	for (const trait_type trait_type : generated_trait_types) {
		success = true;
		while (this->get_trait_count_for_type(trait_type) < defines::get()->get_min_traits_for_type(trait_type) && success) {
			success = this->generate_initial_trait(trait_type);
		}
	}

	success = true;
	const character_attribute target_attribute = this->character->get_skill() != 0 ? this->character->get_primary_attribute() : character_attribute::none;
	const int target_attribute_value = this->character->get_skill();
	while (target_attribute != character_attribute::none && success) {
		for (const trait_type trait_type : generated_trait_types) {
			success = false;

			const int target_attribute_bonus = target_attribute_value - this->get_attribute_value(target_attribute);
			if (target_attribute_bonus == 0) {
				break;
			}

			if (this->get_trait_count_for_type(trait_type) < defines::get()->get_max_traits_for_type(trait_type)) {
				success = this->generate_initial_trait(trait_type);
			}
		}
	}

	if (this->character->get_role() == character_role::ruler && this->get_primary_attribute_value() == 0 && this->character->get_character_type()->get_ruler_modifier() == nullptr && this->character->get_character_type()->get_scaled_ruler_modifier() != nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" is a ruler with a scaled modifier, but has an initial ruler skill of zero.", this->character->get_identifier()));
	} else if (this->character->get_role() == character_role::advisor && this->get_primary_attribute_value() == 0 && this->character->get_advisor_modifier() == nullptr && this->character->get_advisor_effects() == nullptr && this->character->get_character_type()->get_advisor_modifier() == nullptr && this->character->get_character_type()->get_scaled_advisor_modifier() != nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" is an advisor with a scaled modifier, but has an initial advisor skill of zero.", this->character->get_identifier()));
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


void character_game_data::change_attribute_value(const character_attribute attribute, const int change)
{
	if (change == 0) {
		return;
	}

	if (this->is_ruler()) {
		this->apply_ruler_modifier(this->get_country(), -1);
	}
	if (this->is_advisor()) {
		this->apply_advisor_modifier(this->get_country(), -1);
	}
	if (this->character->get_role() == character_role::leader) {
		for (const trait *trait : this->get_traits()) {
			if (trait->get_scaled_leader_modifier() != nullptr && attribute == trait->get_attribute()) {
				this->apply_modifier(trait->get_scaled_leader_modifier(), -std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()));
			}
		}
	}

	const int new_value = (this->attribute_values[attribute] += change);

	if (new_value == 0) {
		this->attribute_values.erase(attribute);
	}

	if (this->is_ruler()) {
		this->apply_ruler_modifier(this->get_country(), 1);
	}
	if (this->is_advisor()) {
		this->apply_advisor_modifier(this->get_country(), 1);
	}
	if (this->character->get_role() == character_role::leader) {
		for (const trait *trait : this->get_traits()) {
			if (trait->get_scaled_leader_modifier() != nullptr && attribute == trait->get_attribute()) {
				this->apply_modifier(trait->get_scaled_leader_modifier(), std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()));
			}
		}
	}
}

int character_game_data::get_primary_attribute_value() const
{
	assert_throw(this->character->get_character_type() != nullptr);

	return this->get_attribute_value(this->character->get_primary_attribute());
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

QVariantList character_game_data::get_traits_of_type(const QString &trait_type_str) const
{
	const trait_type type = enum_converter<trait_type>::to_enum(trait_type_str.toStdString());
	return container::to_qvariant_list(this->get_traits_of_type(type));
}

bool character_game_data::can_have_trait(const trait *trait) const
{
	if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(this->character, read_only_context(this->character))) {
		return false;
	}

	// characters cannot gain a trait which would reduce their primary attribute below 1
	const character_attribute primary_attribute = this->character->get_primary_attribute();
	if (primary_attribute != character_attribute::none) {
		const int trait_primary_attribute_bonus = trait->get_attribute_bonus(primary_attribute);
		if (trait_primary_attribute_bonus < 0 && (this->get_primary_attribute_value() + trait_primary_attribute_bonus) <= 0) {
			return false;
		}
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

	this->on_trait_gained(trait, 1);

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

void character_game_data::remove_trait(const trait *trait)
{
	//remove modifiers that this character is applying on other scopes so that we reapply them later, as the trait change can affect them
	std::erase(this->traits, trait);

	this->on_trait_gained(trait, -1);

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

void character_game_data::on_trait_gained(const trait *trait, const int multiplier)
{
	if (this->is_ruler()) {
		assert_throw(this->get_country() != nullptr);

		if (trait->get_ruler_modifier() != nullptr || trait->get_scaled_ruler_modifier() != nullptr) {
			this->apply_trait_ruler_modifier(trait, this->get_country(), multiplier);
		}
	}
	if (this->is_advisor()) {
		assert_throw(this->get_country() != nullptr);

		if (trait->get_advisor_modifier() != nullptr || trait->get_scaled_advisor_modifier() != nullptr) {
			this->apply_trait_advisor_modifier(trait, this->get_country(), multiplier);
		}
	}

	for (const auto &[attribute, bonus] : trait->get_attribute_bonuses()) {
		this->change_attribute_value(attribute, bonus * multiplier);
	}

	if (trait->get_modifier() != nullptr) {
		this->apply_modifier(trait->get_modifier(), multiplier);
	}

	if (this->character->get_role() == character_role::leader) {
		if (trait->get_leader_modifier() != nullptr) {
			this->apply_modifier(trait->get_leader_modifier(), multiplier);
		}

		if (trait->get_scaled_leader_modifier() != nullptr) {
			this->apply_modifier(trait->get_scaled_leader_modifier(), std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()) * multiplier);
		}
	}

	if (trait->get_military_unit_modifier() != nullptr && this->get_military_unit() != nullptr) {
		this->apply_military_unit_modifier(this->get_military_unit(), multiplier);
	}
}

bool character_game_data::generate_trait(const trait_type trait_type, const character_attribute target_attribute, const int target_attribute_bonus)
{
	std::vector<const trait *> potential_traits;
	int best_attribute_bonus = 0;

	for (const trait *trait : trait::get_all()) {
		if (trait->get_type() != trait_type) {
			continue;
		}

		if (this->has_trait(trait)) {
			continue;
		}

		if (!this->can_have_trait(trait)) {
			continue;
		}

		if (target_attribute != character_attribute::none) {
			const int trait_attribute_bonus = trait->get_attribute_bonus(target_attribute);
			if (trait_attribute_bonus > target_attribute_bonus) {
				continue;
			}

			if (trait_attribute_bonus < best_attribute_bonus) {
				continue;
			} else if (trait_attribute_bonus > best_attribute_bonus) {
				potential_traits.clear();
				best_attribute_bonus = trait_attribute_bonus;
			}
		}

		potential_traits.push_back(trait);
	}

	if (potential_traits.empty()) {
		return false;
	}

	this->add_trait(vector::get_random(potential_traits));
	return true;
}

bool character_game_data::generate_initial_trait(const trait_type trait_type)
{
	const character_attribute target_attribute = this->character->get_skill() != 0 ? this->character->get_primary_attribute() : character_attribute::none;
	const int target_attribute_value = this->character->get_skill();
	const int target_attribute_bonus = target_attribute_value - this->get_attribute_value(target_attribute);

	return this->generate_trait(trait_type, target_attribute, target_attribute_bonus);
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
		this->apply_modifier(modifier->get_modifier(), 1);
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void character_game_data::remove_scripted_modifier(const scripted_character_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		this->apply_modifier(modifier->get_modifier(), -1);
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

bool character_game_data::is_advisor() const
{
	return this->get_country() != nullptr && vector::contains(this->get_country()->get_game_data()->get_advisors(), this->character);
}

std::string character_game_data::get_ruler_modifier_string(const metternich::country *country) const
{
	assert_throw(this->character->get_role() == character_role::ruler);

	std::string str = string::highlight(this->character->get_character_type()->get_name());

	if (this->character->get_character_type()->get_ruler_modifier() != nullptr) {
		str += "\n" + this->character->get_character_type()->get_ruler_modifier()->get_string(country, 1, 1);
	}
	if (this->character->get_character_type()->get_scaled_ruler_modifier() != nullptr) {
		str += "\n" + this->character->get_character_type()->get_scaled_ruler_modifier()->get_string(country, this->get_primary_attribute_value(), 1);
	}

	for (const trait *trait : this->get_traits()) {
		if (trait->get_ruler_modifier() == nullptr && trait->get_scaled_ruler_modifier() == nullptr) {
			continue;
		}

		if (!str.empty()) {
			str += "\n";
		}

		str += string::highlight(trait->get_name());
		if (trait->get_ruler_modifier() != nullptr) {
			str += "\n" + trait->get_ruler_modifier()->get_string(country, 1, 1);
		}
		if (trait->get_scaled_ruler_modifier() != nullptr) {
			str += "\n" + trait->get_scaled_ruler_modifier()->get_string(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()), 1);
		}
	}

	return str;
}

void character_game_data::apply_ruler_modifier(const metternich::country *country, const int multiplier) const
{
	assert_throw(this->character->get_role() == character_role::ruler);
	assert_throw(country != nullptr);

	if (this->character->get_character_type()->get_ruler_modifier() != nullptr) {
		this->character->get_character_type()->get_ruler_modifier()->apply(country, multiplier);
	}
	if (this->character->get_character_type()->get_scaled_ruler_modifier() != nullptr) {
		this->character->get_character_type()->get_scaled_ruler_modifier()->apply(country, this->get_primary_attribute_value() * multiplier);
	}

	for (const trait *trait : this->get_traits()) {
		this->apply_trait_ruler_modifier(trait, country, multiplier);
	}
}

void character_game_data::apply_trait_ruler_modifier(const trait *trait, const metternich::country *country, const int multiplier) const
{
	if (trait->get_ruler_modifier() != nullptr) {
		trait->get_ruler_modifier()->apply(country, multiplier);
	}

	if (trait->get_scaled_ruler_modifier() != nullptr) {
		trait->get_scaled_ruler_modifier()->apply(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()) * multiplier);
	}
}

QString character_game_data::get_advisor_effects_string(const metternich::country *country) const
{
	assert_throw(this->character->get_role() == character_role::advisor);

	std::string str;

	if (this->character->get_advisor_modifier() != nullptr) {
		str += this->character->get_advisor_modifier()->get_string(country);
	}

	if (this->character->get_advisor_effects() != nullptr) {
		if (!str.empty()) {
			str += '\n';
		}

		str += this->character->get_advisor_effects()->get_effects_string(country, read_only_context(country));
	}

	if (!str.empty()) {
		return QString::fromStdString(str);
	}

	if (this->character->get_character_type()->get_advisor_modifier() != nullptr) {
		str = this->character->get_character_type()->get_advisor_modifier()->get_string(country);
	} else if (this->character->get_character_type()->get_scaled_advisor_modifier() != nullptr) {
		str = this->character->get_character_type()->get_scaled_advisor_modifier()->get_string(country, this->get_primary_attribute_value());
	}

	if (this->character->get_character_type()->get_advisor_effects() != nullptr) {
		if (!str.empty()) {
			str += '\n';
		}

		str += this->character->get_character_type()->get_advisor_effects()->get_effects_string(country, read_only_context(country));
	}

	for (const trait *trait : this->get_traits()) {
		if (trait->get_advisor_modifier() == nullptr && trait->get_scaled_advisor_modifier() == nullptr) {
			continue;
		}

		if (trait->get_advisor_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += trait->get_advisor_modifier()->get_string(country);
		}

		if (trait->get_scaled_advisor_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += trait->get_scaled_advisor_modifier()->get_string(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()));
		}
	}

	if (this->get_country() != country) {
		const metternich::character *replaced_advisor = country->get_game_data()->get_replaced_advisor_for(this->character);
		if (replaced_advisor != nullptr) {
			if (!str.empty()) {
				str += '\n';
			}

			str += std::format("Replaces {}", replaced_advisor->get_full_name());
		}
	}

	return QString::fromStdString(str);
}

void character_game_data::apply_advisor_modifier(const metternich::country *country, const int multiplier) const
{
	assert_throw(this->character->get_role() == character_role::advisor);
	assert_throw(country != nullptr);

	if (this->character->get_advisor_effects() != nullptr) {
		return;
	}

	if (this->character->get_advisor_modifier() != nullptr) {
		this->character->get_advisor_modifier()->apply(country, multiplier);
	} else if (this->character->get_character_type()->get_advisor_modifier() != nullptr) {
		this->character->get_character_type()->get_advisor_modifier()->apply(country, multiplier);
	} else if (this->character->get_character_type()->get_scaled_advisor_modifier() != nullptr) {
		this->character->get_character_type()->get_scaled_advisor_modifier()->apply(country, this->get_primary_attribute_value() * multiplier);
	}

	for (const trait *trait : this->get_traits()) {
		this->apply_trait_advisor_modifier(trait, country, multiplier);
	}
}


void character_game_data::apply_trait_advisor_modifier(const trait *trait, const metternich::country *country, const int multiplier) const
{
	if (trait->get_advisor_modifier() != nullptr) {
		trait->get_advisor_modifier()->apply(country, multiplier);
	}

	if (trait->get_scaled_advisor_modifier() != nullptr) {
		trait->get_scaled_advisor_modifier()->apply(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()) * multiplier);
	}
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

void character_game_data::set_commanded_military_unit_stat_modifier(const military_unit_stat stat, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_commanded_military_unit_stat_modifier(stat);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->commanded_military_unit_stat_modifiers.erase(stat);
	} else {
		this->commanded_military_unit_stat_modifiers[stat] = value;
	}
}

void character_game_data::set_commanded_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_commanded_military_unit_type_stat_modifier(type, stat);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->commanded_military_unit_type_stat_modifiers[type].erase(stat);

		if (this->commanded_military_unit_type_stat_modifiers[type].empty()) {
			this->commanded_military_unit_type_stat_modifiers.erase(type);
		}
	} else {
		this->commanded_military_unit_type_stat_modifiers[type][stat] = value;
	}
}

}
