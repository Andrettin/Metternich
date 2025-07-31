#include "metternich.h"

#include "character/character_game_data.h"

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_history.h"
#include "character/character_role.h"
#include "character/character_trait.h"
#include "character/character_trait_type.h"
#include "character/character_type.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_government.h"
#include "country/country_military.h"
#include "country/office.h"
#include "database/defines.h"
#include "game/character_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "script/scripted_character_modifier.h"
#include "species/species.h"
#include "spell/spell.h"
#include "ui/portrait.h"
#include "unit/civilian_unit.h"
#include "unit/civilian_unit_type.h"
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

#include <magic_enum/magic_enum.hpp>

namespace metternich {

character_game_data::character_game_data(const metternich::character *character)
	: character(character)
{
	connect(game::get(), &game::turn_changed, this, &character_game_data::age_changed);
	connect(this, &character_game_data::office_changed, this, &character_game_data::titled_name_changed);

	this->portrait = this->character->get_portrait();
}

void character_game_data::apply_history()
{
	const character_history *character_history = this->character->get_history();

	const metternich::country *country = character_history->get_country();

	if ((this->character->has_role(character_role::advisor) || this->character->has_role(character_role::leader) || this->character->has_role(character_role::civilian)) && country != nullptr && !country->get_game_data()->is_under_anarchy()) {
		country_game_data *country_game_data = country->get_game_data();
		country_government *country_government = country->get_government();
		const technology *obsolescence_technology = this->character->get_obsolescence_technology();

		if (this->character->get_required_technology() != nullptr) {
			country_game_data->add_technology_with_prerequisites(this->character->get_required_technology());
		}

		if (obsolescence_technology != nullptr && country_game_data->has_technology(obsolescence_technology)) {
			this->set_dead(true);
		} else {
			if (this->character->has_role(character_role::advisor)) {
				if (country_government->can_have_advisors() && !country_government->has_incompatible_advisor_to(this->character)) {
					country_government->add_advisor(this->character);
				}
			} else if (this->character->has_role(character_role::leader)) {
				const province *deployment_province = character_history->get_deployment_province();
				if (deployment_province == nullptr && country_game_data->get_capital_province() != nullptr) {
					deployment_province = country_game_data->get_capital_province();
				}

				assert_throw(deployment_province != nullptr);

				if (deployment_province->is_water_zone() || deployment_province->get_game_data()->get_owner() == country) {
					assert_throw(country != nullptr);
					this->deploy_to_province(country, deployment_province);
				}
			} else if (this->character->has_role(character_role::civilian)) {
				const site *deployment_site = character_history->get_deployment_site();
				if (deployment_site == nullptr && country_game_data->get_capital() != nullptr) {
					deployment_site = country_game_data->get_capital();
				}

				assert_throw(deployment_site != nullptr);

				if (!deployment_site->get_game_data()->is_on_map()) {
					return;
				}

				if (deployment_site->get_game_data()->get_owner() != country) {
					return;
				}

				const civilian_unit_type *type = this->character->get_civilian_unit_type();
				assert_throw(type != nullptr);

				if (type->get_required_technology() != nullptr) {
					country_game_data->add_technology_with_prerequisites(type->get_required_technology());
				}

				QPoint tile_pos = deployment_site->get_game_data()->get_tile_pos();

				if (map::get()->get_tile(tile_pos)->get_civilian_unit() != nullptr) {
					const std::optional<QPoint> nearest_tile_pos = map::get()->get_nearest_available_tile_pos_for_civilian_unit(tile_pos);
					if (!nearest_tile_pos.has_value()) {
						log::log_error(std::format("Cannot deploy civilian character \"{}\", since the tile of its deployment site (\"{}\") is already occupied by another civilian unit, and so are any valid tiles near it.", this->character->get_identifier(), deployment_site->get_identifier()));
						return;
					}

					tile_pos = nearest_tile_pos.value();
				}

				auto civilian_unit = make_qunique<metternich::civilian_unit>(this->character, country);
				civilian_unit->set_tile_pos(tile_pos);

				country_game_data->add_civilian_unit(std::move(civilian_unit));
			}
		}
	}
}

void character_game_data::on_setup_finished()
{
	if (this->character->get_species()->get_modifier() != nullptr) {
		this->character->get_species()->get_modifier()->apply(this->character, 1);
	}

	for (const character_trait *trait : this->character->get_traits()) {
		this->add_trait(trait);
	}

	std::vector<character_trait_type> generated_trait_types{ character_trait_type::background, character_trait_type::personality, character_trait_type::expertise };
	if (this->character->has_role(character_role::ruler)) {
		generated_trait_types.insert(generated_trait_types.begin(), character_trait_type::ruler);
	} else if (this->character->has_role(character_role::advisor)) {
		generated_trait_types.insert(generated_trait_types.begin(), character_trait_type::advisor);
	} else if (this->character->has_role(character_role::governor)) {
		generated_trait_types.insert(generated_trait_types.begin(), character_trait_type::governor);
	}

	bool success = true;
	for (const character_trait_type trait_type : generated_trait_types) {
		success = true;
		while (this->get_trait_count_for_type(trait_type) < defines::get()->get_min_character_traits_for_type(trait_type) && success) {
			success = this->generate_initial_trait(trait_type);
		}
	}

	success = true;
	const character_attribute target_attribute = this->character->get_skill() != 0 ? this->character->get_primary_attribute() : character_attribute::none;
	const int target_attribute_value = this->character->get_skill();
	while (target_attribute != character_attribute::none && success) {
		for (const character_trait_type trait_type : generated_trait_types) {
			success = false;

			const int target_attribute_bonus = target_attribute_value - this->get_attribute_value(target_attribute);
			if (target_attribute_bonus == 0) {
				break;
			}

			if (this->get_trait_count_for_type(trait_type) < defines::get()->get_max_character_traits_for_type(trait_type)) {
				success = this->generate_initial_trait(trait_type);
			}
		}
	}

	if (this->character->has_role(character_role::ruler)) {
		for (const character_trait *trait : this->get_traits_of_type(character_trait_type::ruler)) {
			if (trait->get_office_modifier(defines::get()->get_ruler_office()) == nullptr && trait->get_scaled_office_modifier(defines::get()->get_ruler_office()) != nullptr && this->get_attribute_value(trait->get_attribute()) == 0) {
				throw std::runtime_error(std::format("Character \"{}\" is a ruler with a scaled modifier for trait \"{}\", but has an initial value of zero for that trait's attribute.", this->character->get_identifier(), trait->get_identifier()));
			}
		}
	} else if (this->character->has_role(character_role::advisor)) {
		for (const character_trait *trait : this->get_traits_of_type(character_trait_type::advisor)) {
			if (trait->get_advisor_modifier() == nullptr && trait->get_advisor_effects() == nullptr && trait->get_scaled_advisor_modifier() != nullptr && this->get_attribute_value(trait->get_attribute()) == 0) {
				throw std::runtime_error(std::format("Character \"{}\" is an advisor with a scaled modifier for trait \"{}\", but has an initial value of zero for that trait's attribute.", this->character->get_identifier(), trait->get_identifier()));
			}
		}
	} else if (this->character->has_role(character_role::governor)) {
		for (const character_trait *trait : this->get_traits_of_type(character_trait_type::governor)) {
			if (trait->get_governor_modifier() == nullptr && trait->get_scaled_governor_modifier() != nullptr && this->get_attribute_value(trait->get_attribute()) == 0) {
				throw std::runtime_error(std::format("Character \"{}\" is a governor with a scaled modifier for trait \"{}\", but has an initial value of zero for that trait's attribute.", this->character->get_identifier(), trait->get_identifier()));
			}
		}
	}

	this->check_portrait();
}

std::string character_game_data::get_titled_name() const
{
	if (this->get_office() != nullptr) {
		return std::format("{} {}", this->get_country()->get_government()->get_office_title_name(this->get_office()), this->character->get_full_name());
	}

	if (this->is_governor()) {
		return std::format("{} {}", this->character->get_governable_province()->get_game_data()->get_governor_title_name(), this->character->get_full_name());
	}

	if (this->is_landholder()) {
		return std::format("{} {}", this->character->get_holdable_site()->get_game_data()->get_landholder_title_name(), this->character->get_full_name());
	}

	return this->character->get_full_name();
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
	if (!this->character->has_role(character_role::ruler) && !this->character->has_role(character_role::advisor) && !this->character->has_role(character_role::governor) && !this->character->has_role(character_role::landholder)) {
		//only rulers, advisors, governors and landholders need portraits
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
	if (this->get_office() != nullptr) {
		this->get_country()->get_government()->on_office_holder_died(this->get_office(), this->character);
	}

	this->set_country(nullptr);
	this->set_dead(true);
}

void character_game_data::change_attribute_value(const character_attribute attribute, const int change)
{
	if (change == 0) {
		return;
	}

	if (this->get_office() != nullptr) {
		this->apply_office_modifier(this->country, this->get_office(), -1);
	}
	if (this->is_advisor()) {
		this->apply_advisor_modifier(this->get_country(), -1);
	}
	if (this->is_governor()) {
		this->apply_governor_modifier(this->character->get_governable_province(), -1);
	}
	if (this->is_landholder()) {
		this->apply_landholder_modifier(this->character->get_holdable_site(), -1);
	}
	if (this->character->has_role(character_role::leader)) {
		for (const character_trait *trait : this->get_traits()) {
			if (trait->get_scaled_leader_modifier() != nullptr && attribute == trait->get_attribute()) {
				this->apply_modifier(trait->get_scaled_leader_modifier(), -std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()));
			}
		}
	}

	const int new_value = (this->attribute_values[attribute] += change);

	if (new_value == 0) {
		this->attribute_values.erase(attribute);
	}

	if (this->get_office() != nullptr) {
		this->apply_office_modifier(this->country, this->get_office(), 1);
	}
	if (this->is_advisor()) {
		this->apply_advisor_modifier(this->get_country(), 1);
	}
	if (this->is_governor()) {
		this->apply_governor_modifier(this->character->get_governable_province(), 1);
	}
	if (this->is_landholder()) {
		this->apply_landholder_modifier(this->character->get_holdable_site(), 1);
	}
	if (this->character->has_role(character_role::leader)) {
		for (const character_trait *trait : this->get_traits()) {
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

std::set<character_attribute> character_game_data::get_main_attributes() const
{
	std::set<character_attribute> attributes;

	if (this->character->get_primary_attribute() != character_attribute::none) {
		attributes.insert(this->character->get_primary_attribute());
	}

	for (const character_trait *trait : this->get_traits()) {
		if (trait->get_attribute() != character_attribute::none) {
			attributes.insert(trait->get_attribute());
		}
	}

	return attributes;
}

QVariantList character_game_data::get_traits_qvariant_list() const
{
	return container::to_qvariant_list(this->get_traits());
}

std::vector<const character_trait *> character_game_data::get_traits_of_type(const character_trait_type trait_type) const
{
	std::vector<const character_trait *> traits;

	for (const character_trait *trait : this->get_traits()) {
		if (!trait->get_types().contains(trait_type)) {
			continue;
		}

		traits.push_back(trait);
	}

	return traits;
}

QVariantList character_game_data::get_traits_of_type(const QString &trait_type_str) const
{
	const character_trait_type type = magic_enum::enum_cast<character_trait_type>(trait_type_str.toStdString()).value();
	return container::to_qvariant_list(this->get_traits_of_type(type));
}

bool character_game_data::can_have_trait(const character_trait *trait) const
{
	if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(this->character, read_only_context(this->character))) {
		return false;
	}

	return true;
}

bool character_game_data::can_gain_trait(const character_trait *trait) const
{
	if (this->has_trait(trait)) {
		return false;
	}

	if (!this->can_have_trait(trait)) {
		return false;
	}

	for (const character_trait_type trait_type : trait->get_types()) {
		if (trait_type == character_trait_type::ruler && !this->character->has_role(character_role::ruler)) {
			continue;
		}

		if (trait_type == character_trait_type::advisor && !this->character->has_role(character_role::advisor)) {
			continue;
		}

		if (trait_type == character_trait_type::governor && !this->character->has_role(character_role::governor)) {
			continue;
		}

		if (this->get_trait_count_for_type(trait_type) >= defines::get()->get_max_character_traits_for_type(trait_type)) {
			return false;
		}
	}

	// characters cannot gain a trait which would reduce their main attributes below 1
	for (const character_attribute attribute : this->get_main_attributes()) {
		const int trait_primary_attribute_bonus = trait->get_attribute_bonus(attribute);
		if (trait_primary_attribute_bonus < 0 && (this->get_attribute_value(attribute) + trait_primary_attribute_bonus) <= 0) {
			return false;
		}
	}

	return true;
}

bool character_game_data::has_trait(const character_trait *trait) const
{
	return vector::contains(this->get_traits(), trait);
}

void character_game_data::add_trait(const character_trait *trait)
{
	if (this->has_trait(trait)) {
		log::log_error(std::format("Tried to add trait \"{}\" to character \"{}\", but they already have the trait.", trait->get_identifier(), this->character->get_identifier()));
		return;
	}

	if (!this->can_gain_trait(trait)) {
		log::log_error(std::format("Tried to add trait \"{}\" to character \"{}\", who cannot gain it.", trait->get_identifier(), this->character->get_identifier()));
		return;
	}

	this->traits.push_back(trait);

	this->on_trait_gained(trait, 1);

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

void character_game_data::remove_trait(const character_trait *trait)
{
	//remove modifiers that this character is applying on other scopes so that we reapply them later, as the trait change can affect them
	std::erase(this->traits, trait);

	this->on_trait_gained(trait, -1);

	this->sort_traits();

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

void character_game_data::on_trait_gained(const character_trait *trait, const int multiplier)
{
	if (this->get_office() != nullptr) {
		assert_throw(this->get_country() != nullptr);

		if (trait->get_office_modifier(this->get_office()) != nullptr || trait->get_scaled_office_modifier(this->get_office()) != nullptr) {
			this->apply_trait_office_modifier(trait, this->get_country(), this->get_office(), multiplier);
		}
	}

	if (this->is_advisor()) {
		assert_throw(this->get_country() != nullptr);

		if (trait->get_advisor_modifier() != nullptr || trait->get_scaled_advisor_modifier() != nullptr) {
			this->apply_trait_advisor_modifier(trait, this->get_country(), multiplier);
		}

		if (trait->get_advisor_effects() != nullptr && multiplier > 0) {
			context ctx(this->get_country());
			trait->get_advisor_effects()->do_effects(this->get_country(), ctx);
		}
	}
	if (this->is_governor()) {
		assert_throw(this->get_country() != nullptr);

		if (trait->get_governor_modifier() != nullptr || trait->get_scaled_governor_modifier() != nullptr) {
			this->apply_trait_governor_modifier(trait, this->character->get_governable_province(), multiplier);
		}
	}

	for (const auto &[attribute, bonus] : trait->get_attribute_bonuses()) {
		this->change_attribute_value(attribute, bonus * multiplier);
	}

	if (trait->get_modifier() != nullptr) {
		this->apply_modifier(trait->get_modifier(), multiplier);
	}

	if (this->character->has_role(character_role::leader)) {
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

bool character_game_data::generate_trait(const character_trait_type trait_type, const character_attribute target_attribute, const int target_attribute_bonus)
{
	std::vector<const character_trait *> potential_traits;
	int best_attribute_bonus = 0;

	for (const character_trait *trait : character_trait::get_all()) {
		if (!trait->get_types().contains(trait_type)) {
			continue;
		}

		if (!this->can_gain_trait(trait)) {
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

bool character_game_data::generate_initial_trait(const character_trait_type trait_type)
{
	const character_attribute target_attribute = this->character->get_skill() != 0 ? this->character->get_primary_attribute() : character_attribute::none;
	const int target_attribute_value = this->character->get_skill();
	const int target_attribute_bonus = target_attribute_value - this->get_attribute_value(target_attribute);

	return this->generate_trait(trait_type, target_attribute, target_attribute_bonus);
}

void character_game_data::sort_traits()
{
	std::sort(this->traits.begin(), this->traits.end(), [](const character_trait *lhs, const character_trait *rhs) {
		if (*lhs->get_types().begin() != *rhs->get_types().begin()) {
			return *lhs->get_types().begin() < *rhs->get_types().begin();
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
	return this->get_country() != nullptr && this->get_country()->get_government()->get_ruler() == this->character;
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

std::string character_game_data::get_office_modifier_string(const metternich::country *country, const metternich::office *office) const
{
	if (office->is_ruler()) {
		assert_throw(this->character->has_role(character_role::ruler));

		if (defines::get()->get_ruler_traits_game_rule() != nullptr && !game::get()->get_rules()->get_value(defines::get()->get_ruler_traits_game_rule())) {
			return {};
		}
	} else {
		assert_throw(this->character->has_role(character_role::advisor));
	}

	assert_throw(office != nullptr);

	std::string str;

	for (const character_trait *trait : this->get_traits()) {
		if (trait->get_office_modifier(office) == nullptr && trait->get_scaled_office_modifier(office) == nullptr) {
			continue;
		}

		const size_t indent = trait->has_hidden_name() ? 0 : 1;

		std::string trait_modifier_str;

		if (trait->get_office_modifier(office) != nullptr) {
			trait_modifier_str = trait->get_office_modifier(office)->get_string(country, 1, indent);
		}

		if (trait->get_scaled_office_modifier(office) != nullptr) {
			trait_modifier_str = trait->get_scaled_office_modifier(office)->get_string(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()), indent);
		}

		if (trait_modifier_str.empty()) {
			continue;
		}

		if (!str.empty()) {
			str += "\n";
		}

		if (!trait->has_hidden_name()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += string::highlight(trait->get_name());
		}

		str += "\n" + trait_modifier_str;
	}

	return str;
}

void character_game_data::apply_office_modifier(const metternich::country *country, const metternich::office *office, const int multiplier) const
{
	if (office->is_ruler()) {
		assert_throw(this->character->has_role(character_role::ruler));
	} else {
		assert_throw(this->character->has_role(character_role::advisor));
	}

	assert_throw(country != nullptr);
	assert_throw(office != nullptr);

	for (const character_trait *trait : this->get_traits()) {
		this->apply_trait_office_modifier(trait, country, office, multiplier);
	}
}

void character_game_data::apply_trait_office_modifier(const character_trait *trait, const metternich::country *country, const metternich::office *office, const int multiplier) const
{
	if (office->is_ruler() && defines::get()->get_ruler_traits_game_rule() != nullptr && !game::get()->get_rules()->get_value(defines::get()->get_ruler_traits_game_rule())) {
		return;
	}

	if (trait->get_office_modifier(office) != nullptr) {
		trait->get_office_modifier(office)->apply(country, multiplier);
	}

	if (trait->get_scaled_office_modifier(office) != nullptr) {
		trait->get_scaled_office_modifier(office)->apply(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()) * multiplier);
	}
}

bool character_game_data::is_advisor() const
{
	return this->get_country() != nullptr && vector::contains(this->get_country()->get_government()->get_advisors(), this->character);
}

QString character_game_data::get_advisor_effects_string(const metternich::country *country) const
{
	assert_throw(this->character->has_role(character_role::advisor));

	std::string str;

	for (const character_trait *trait : this->get_traits()) {
		if (trait->get_advisor_modifier() == nullptr && trait->get_scaled_advisor_modifier() == nullptr && trait->get_advisor_effects() == nullptr) {
			continue;
		}

		if (!trait->has_hidden_name()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += string::highlight(trait->get_name());
		}

		const size_t indent = trait->has_hidden_name() ? 0 : 1;

		if (trait->get_advisor_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += trait->get_advisor_modifier()->get_string(country, 1, indent);
		}

		if (trait->get_scaled_advisor_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += trait->get_scaled_advisor_modifier()->get_string(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()), indent);
		}

		if (trait->get_advisor_effects() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += trait->get_advisor_effects()->get_effects_string(country, read_only_context(country), indent);
		}
	}

	if (this->get_country() != country) {
		const metternich::character *replaced_advisor = country->get_government()->get_replaced_advisor_for(this->character);
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
	assert_throw(this->character->has_role(character_role::advisor));
	assert_throw(country != nullptr);

	for (const character_trait *trait : this->get_traits()) {
		this->apply_trait_advisor_modifier(trait, country, multiplier);
	}
}

void character_game_data::apply_trait_advisor_modifier(const character_trait *trait, const metternich::country *country, const int multiplier) const
{
	if (trait->get_advisor_modifier() != nullptr) {
		trait->get_advisor_modifier()->apply(country, multiplier);
	}

	if (trait->get_scaled_advisor_modifier() != nullptr) {
		trait->get_scaled_advisor_modifier()->apply(country, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()) * multiplier);
	}
}

bool character_game_data::is_governor() const
{
	return this->character->get_governable_province() != nullptr && this->character->get_governable_province()->get_game_data()->get_governor() == this->character;
}

std::string character_game_data::get_governor_modifier_string(const metternich::province *province) const
{
	assert_throw(this->character->has_role(character_role::governor));

	std::string str;

	for (const character_trait *trait : this->get_traits()) {
		if (trait->get_governor_modifier() == nullptr && trait->get_scaled_governor_modifier() == nullptr) {
			continue;
		}

		if (!str.empty()) {
			str += "\n";
		}

		str += string::highlight(trait->get_name());
		if (trait->get_governor_modifier() != nullptr) {
			str += "\n" + trait->get_governor_modifier()->get_string(province, 1, 1);
		}
		if (trait->get_scaled_governor_modifier() != nullptr) {
			str += "\n" + trait->get_scaled_governor_modifier()->get_string(province, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()), 1);
		}
	}

	return str;
}

void character_game_data::apply_governor_modifier(const metternich::province *province, const int multiplier) const
{
	assert_throw(this->character->has_role(character_role::governor));
	assert_throw(province != nullptr);

	for (const character_trait *trait : this->get_traits()) {
		this->apply_trait_governor_modifier(trait, province, multiplier);
	}
}

void character_game_data::apply_trait_governor_modifier(const character_trait *trait, const metternich::province *province, const int multiplier) const
{
	if (trait->get_governor_modifier() != nullptr) {
		trait->get_governor_modifier()->apply(province, multiplier);
	}

	if (trait->get_scaled_governor_modifier() != nullptr) {
		trait->get_scaled_governor_modifier()->apply(province, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()) * multiplier);
	}
}

bool character_game_data::is_landholder() const
{
	return this->character->get_holdable_site() != nullptr && this->character->get_holdable_site()->get_game_data()->get_landholder() == this->character;
}

std::string character_game_data::get_landholder_modifier_string(const metternich::site *site) const
{
	assert_throw(this->character->has_role(character_role::landholder));
	assert_throw(site != nullptr);

	std::string str;

	if (defines::get()->get_scaled_landholder_modifier() != nullptr) {
		str = defines::get()->get_scaled_landholder_modifier()->get_string(site, this->get_attribute_value(character_attribute::stewardship));
	}

	return str;
}

void character_game_data::apply_landholder_modifier(const metternich::site *site, const int multiplier) const
{
	assert_throw(this->character->has_role(character_role::landholder));
	assert_throw(site != nullptr);

	if (defines::get()->get_scaled_landholder_modifier() != nullptr) {
		defines::get()->get_scaled_landholder_modifier()->apply(site, this->get_attribute_value(character_attribute::stewardship) * multiplier);
	}
}

bool character_game_data::is_deployable() const
{
	if (this->character->get_military_unit_category() == military_unit_category::none) {
		return false;
	}

	return true;
}

void character_game_data::deploy_to_province(const metternich::country *country, const province *province)
{
	assert_throw(country != nullptr);
	assert_throw(province != nullptr);
	assert_throw(!this->is_deployed());
	assert_throw(this->is_deployable());

	const military_unit_type *military_unit_type = country->get_military()->get_best_military_unit_category_type(this->character->get_military_unit_category(), this->character->get_culture());

	auto military_unit = make_qunique<metternich::military_unit>(military_unit_type, country, this->character);

	assert_throw(military_unit->can_move_to(province));

	military_unit->set_province(province);

	country->get_military()->add_military_unit(std::move(military_unit));

	assert_throw(this->get_country() != nullptr);
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
	for (const character_trait *trait : this->get_traits()) {
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
