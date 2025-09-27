#include "metternich.h"

#include "character/character_game_data.h"

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_history.h"
#include "character/character_trait.h"
#include "character/character_trait_type.h"
#include "character/monster_type.h"
#include "character/saving_throw_type.h"
#include "character/skill.h"
#include "database/defines.h"
#include "domain/country_government.h"
#include "domain/country_military.h"
#include "domain/country_technology.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/office.h"
#include "engine_interface.h"
#include "game/character_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "item/enchantment.h"
#include "item/item.h"
#include "item/item_slot.h"
#include "item/item_type.h"
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
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/string_conversion_util.h"
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

void character_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "portrait") {
		this->portrait = portrait::get(value);
	} else if (key == "domain") {
		this->domain = domain::get(value);
	} else if (key == "dead") {
		this->dead = string::to_bool(value);
	} else if (key == "character_class") {
		this->character_class = character_class::get(value);
	} else if (key == "level") {
		this->level = std::stoi(value);
	} else if (key == "experience") {
		this->experience = std::stoll(value);
	} else if (key == "hit_dice_count") {
		this->hit_dice_count = std::stoi(value);
	} else if (key == "hit_points") {
		this->hit_points = std::stoi(value);
	} else if (key == "max_hit_points") {
		this->max_hit_points = std::stoi(value);
	} else if (key == "armor_class_bonus") {
		this->armor_class_bonus = std::stoi(value);
	} else if (key == "to_hit_bonus") {
		this->to_hit_bonus = std::stoi(value);
	} else {
		throw std::runtime_error(std::format("Invalid character game data property: \"{}\".", key));
	}
}

void character_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "attributes") {
		scope.for_each_property([&](const gsml_property &attribute_property) {
			this->attribute_values[character_attribute::get(attribute_property.get_key())] = std::stoi(attribute_property.get_value());
		});
	} else if (tag == "species_armor_class_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			this->species_armor_class_bonuses[species::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "saving_throw_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			this->saving_throw_bonuses[saving_throw_type::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "skill_trainings") {
		scope.for_each_property([&](const gsml_property &property) {
			this->skill_trainings[skill::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "skill_values") {
		scope.for_each_property([&](const gsml_property &property) {
			this->skill_values[skill::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "items") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			auto item = make_qunique<metternich::item>(child_scope);
			child_scope.process(item.get());
			metternich::item *item_ptr = item.get();
			this->items.push_back(std::move(item));

			if (item_ptr->is_equipped()) {
				this->equipped_items[item_ptr->get_slot()].push_back(item_ptr);
			}
		});
	} else {
		throw std::runtime_error(std::format("Invalid character game data scope: \"{}\".", tag));
	}
}

gsml_data character_game_data::to_gsml_data() const
{
	gsml_data data(this->character->get_identifier());

	data.add_property("portrait", this->get_portrait()->get_identifier());

	if (this->get_domain() != nullptr) {
		data.add_property("domain", this->get_domain()->get_identifier());
	}

	data.add_property("dead", string::from_bool(this->is_dead()));
	if (this->get_character_class() != nullptr) {
		data.add_property("character_class", this->get_character_class()->get_identifier());
	}
	data.add_property("level", std::to_string(this->get_level()));
	data.add_property("experience", std::to_string(this->get_experience()));

	if (!this->attribute_values.empty()) {
		gsml_data attributes_data("attributes");
		for (const auto &[attribute, value] : this->attribute_values) {
			attributes_data.add_property(attribute->get_identifier(), std::to_string(value));
		}
		data.add_child(std::move(attributes_data));
	}

	data.add_property("hit_dice_count", std::to_string(this->get_hit_dice_count()));
	data.add_property("hit_points", std::to_string(this->get_hit_points()));
	data.add_property("max_hit_points", std::to_string(this->get_max_hit_points()));
	data.add_property("armor_class_bonus", std::to_string(this->get_armor_class_bonus()));
	data.add_property("to_hit_bonus", std::to_string(this->get_to_hit_bonus()));

	if (!this->species_armor_class_bonuses.empty()) {
		gsml_data species_armor_class_bonuses_data("species_armor_class_bonuses");
		for (const auto &[species, bonus] : this->species_armor_class_bonuses) {
			species_armor_class_bonuses_data.add_property(species->get_identifier(), std::to_string(bonus));
		}
		data.add_child(std::move(species_armor_class_bonuses_data));
	}

	if (!this->saving_throw_bonuses.empty()) {
		gsml_data saving_throw_bonuses_data("saving_throw_bonuses");
		for (const auto &[saving_throw_type, bonus] : this->saving_throw_bonuses) {
			saving_throw_bonuses_data.add_property(saving_throw_type->get_identifier(), std::to_string(bonus));
		}
		data.add_child(std::move(saving_throw_bonuses_data));
	}

	if (!this->skill_trainings.empty()) {
		gsml_data skill_trainings_data("skill_trainings");
		for (const auto &[skill, training] : this->skill_trainings) {
			skill_trainings_data.add_property(skill->get_identifier(), std::to_string(training));
		}
		data.add_child(std::move(skill_trainings_data));
	}

	if (!this->skill_values.empty()) {
		gsml_data skill_values_data("skill_values");
		for (const auto &[skill, value] : this->skill_values) {
			skill_values_data.add_property(skill->get_identifier(), std::to_string(value));
		}
		data.add_child(std::move(skill_values_data));
	}

	if (!this->items.empty()) {
		gsml_data items_data("items");
		for (const qunique_ptr<item> &item : this->items) {
			items_data.add_child(item->to_gsml_data());
		}
		data.add_child(std::move(items_data));
	}

	return data;
}

void character_game_data::apply_species_and_class(const int level)
{
	const species *species = this->character->get_species();
	if (species->get_modifier() != nullptr) {
		species->get_modifier()->apply(this->character);
	}

	const culture *culture = this->character->get_culture();
	if (culture != nullptr && culture->get_character_modifier() != nullptr) {
		culture->get_character_modifier()->apply(this->character);
	}

	const monster_type *monster_type = this->character->get_monster_type();
	if (monster_type != nullptr) {
		if (!monster_type->get_hit_dice().is_null()) {
			this->apply_hit_dice(monster_type->get_hit_dice());
		}

		if (monster_type->get_modifier() != nullptr) {
			monster_type->get_modifier()->apply(this->character);
		}
	}

	const metternich::character_class *character_class = this->get_character_class();
	for (const character_attribute *attribute : character_attribute::get_all()) {
		int min_result = 1;
		min_result = std::max(species->get_min_attribute_value(attribute), min_result);
		if (character_class != nullptr) {
			min_result = std::max(character_class->get_min_attribute_value(attribute), min_result);
		}

		int max_result = std::numeric_limits<int>::max();
		if (species->get_max_attribute_value(attribute) != 0) {
			max_result = std::min(species->get_max_attribute_value(attribute), max_result);
		}

		assert_throw(max_result >= min_result);

		static constexpr dice attribute_dice(3, 6);
		const int minimum_possible_result = attribute_dice.get_minimum_result() + this->get_attribute_value(attribute);
		const int maximum_possible_result = attribute_dice.get_maximum_result() + this->get_attribute_value(attribute);
		if ((maximum_possible_result < min_result || minimum_possible_result > max_result) && character_class != nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" of species \"{}\" cannot be generated with character class \"{}\", since it cannot possibly fulfill the attribute requirements.", this->character->get_identifier(), species->get_identifier(), character_class->get_identifier()));
		}

		bool valid_result = false;
		while (!valid_result) {
			const int base_result = random::get()->roll_dice(attribute_dice);
			const int result = base_result + this->get_attribute_value(attribute);

			valid_result = result >= min_result && result <= max_result;
			if (valid_result) {
				this->change_attribute_value(attribute, base_result);
			}
		}
	}

	if (character_class != nullptr) {
		this->set_level(std::min(level, character_class->get_max_level()));

		data_entry_set<item_slot> filled_item_slots;
		for (const qunique_ptr<item> &item : this->get_items()) {
			if (item->get_slot() != nullptr) {
				filled_item_slots.insert(item->get_slot());

				//FIXME: if the item is a two-handed weapon, it should also add mark the shield slot as filled
			}
		}

		for (const item_type *starting_item_type : character_class->get_starting_items()) {
			if (starting_item_type->get_slot() != nullptr && filled_item_slots.contains(starting_item_type->get_slot())) {
				continue;
			}

			auto item = make_qunique<metternich::item>(starting_item_type, nullptr, nullptr);
			this->add_item(std::move(item));
		}
	}
}

void character_game_data::apply_history(const QDate &start_date)
{
	const character_history *character_history = this->character->get_history();

	this->character_class = this->character->get_character_class();
	const int level = std::max(character_history->get_level(), 1);
	this->apply_species_and_class(level);

	if (start_date < this->character->get_start_date()) {
		return;
	}

	if (this->character->get_death_date().isValid() && start_date >= this->character->get_death_date()) {
		this->set_dead(true);
		return;
	}

	const metternich::domain *domain = character_history->get_country();

	if (domain != nullptr && !domain->get_game_data()->is_under_anarchy()) {
		this->set_domain(domain);

		if (this->character->get_military_unit_category() != military_unit_category::none) {
			const province *deployment_province = character_history->get_deployment_province();
			if (deployment_province != nullptr) {
				if (deployment_province->is_water_zone() || deployment_province->get_game_data()->get_owner() == domain) {
					this->deploy_to_province(domain, deployment_province);
				}
			}
		}
	}

	if (this->get_domain() == nullptr && this->character->get_home_site() != nullptr) {
		this->set_domain(this->character->get_home_site()->get_game_data()->get_owner());
	}
}

void character_game_data::on_setup_finished()
{
	for (const character_trait *trait : this->character->get_traits()) {
		this->add_trait(trait);
	}

	std::vector<character_trait_type> generated_trait_types{ character_trait_type::background, character_trait_type::personality, character_trait_type::expertise };

	bool success = true;
	for (const character_trait_type trait_type : generated_trait_types) {
		success = true;
		while (this->get_trait_count_for_type(trait_type) < defines::get()->get_min_character_traits_for_type(trait_type) && success) {
			success = this->generate_initial_trait(trait_type);
		}
	}

	success = true;
	const character_attribute *target_attribute = this->character->get_skill() != 0 ? this->character->get_primary_attribute() : nullptr;
	const int target_attribute_value = this->character->get_skill();
	while (target_attribute != nullptr && success) {
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

	this->check_portrait();
}

std::string character_game_data::get_titled_name() const
{
	if (this->get_office() != nullptr) {
		assert_throw(this->get_domain() != nullptr);
		return std::format("{} {}", this->get_domain()->get_government()->get_office_title_name(this->get_office()), this->character->get_full_name());
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
	if (this->is_current_portrait_valid() || this->character->is_temporary()) {
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

void character_game_data::set_domain(const metternich::domain *domain)
{
	if (domain == this->get_domain()) {
		return;
	}

	if (this->get_domain() != nullptr) {
		this->get_domain()->get_game_data()->remove_character(this->character);
	}

	this->domain = domain;

	if (this->get_domain() != nullptr) {
		this->get_domain()->get_game_data()->add_character(this->character);
	}

	if (game::get()->is_running()) {
		emit domain_changed();
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
	this->set_dead(true);

	if (this->get_office() != nullptr) {
		assert_throw(this->get_domain() != nullptr);
		this->get_domain()->get_government()->on_office_holder_died(this->get_office(), this->character);
	}

	if (this->get_military_unit() != nullptr) {
		this->get_domain()->get_military()->on_leader_died(this->character);
	}

	assert_throw(this->get_office() == nullptr);
	this->set_domain(nullptr);
}

bool character_game_data::exists() const
{
	//whether the character exists in the game
	return this->get_domain() != nullptr;
}

bool character_game_data::has_ever_existed() const
{
	//whether the character exists in the game or the scenario history
	if (this->exists()) {
		return true;
	}

	//if (this->character->get_home_site() != nullptr && !this->character->get_home_site()->get_game_data()->is_on_map()) {
	//	return false;
	//}

	return this->is_dead();
}

std::vector<const metternich::character *> character_game_data::get_children() const
{
	std::vector<const metternich::character *> children;

	for (const character_base *child_base : this->character->get_children()) {
		const metternich::character *child = static_cast<const metternich::character *>(child_base);
		if (child->get_game_data()->has_ever_existed()) {
			children.push_back(child);
		}
	}

	return children;
}

const metternich::character_class *character_game_data::get_character_class() const
{
	return this->character_class;
}

void character_game_data::set_character_class(const metternich::character_class *character_class)
{
	if (character_class == this->get_character_class()) {
		return;
	}

	assert_throw(character_class != nullptr);
	assert_throw(character_class->is_allowed_for_species(this->character->get_species()));

	this->character_class = character_class;

	if (game::get()->is_running()) {
		emit character_class_changed();
	}
}

int character_game_data::get_level() const
{
	return this->level;
}

void character_game_data::set_level(const int level)
{
	const int old_level = this->get_level();
	if (level == old_level) {
		return;
	}

	//characters losing levels is not supported
	assert_throw(level > old_level);
	assert_throw(this->get_character_class() != nullptr);
	assert_throw(level <= this->get_character_class()->get_max_level());

	this->level = level;

	for (int i = old_level + 1; i <= level; ++i) {
		this->on_level_gained(i, 1);
	}

	const int64_t level_experience = this->get_experience_for_level(this->get_level());
	if (this->get_experience() < level_experience) {
		this->set_experience(level_experience);
	}

	if (game::get()->is_running()) {
		emit level_changed();
	}
}

void character_game_data::change_level(const int change)
{
	this->set_level(this->get_level() + change);
}

void character_game_data::on_level_gained(const int affected_level, const int multiplier)
{
	//only the effects of one level at a time should be applied
	assert_throw(std::abs(multiplier) == 1);

	//only gaining levels is supported
	assert_throw(multiplier > 0);

	assert_throw(affected_level >= 1);

	const metternich::character_class *character_class = this->get_character_class();
	assert_throw(character_class != nullptr);

	if (game::get()->is_running() && this->character == game::get()->get_player_character()) {
		const std::string level_effects_string = character_class->get_level_effects_string(affected_level, this->character);

		engine_interface::get()->add_notification("Level Up", this->get_portrait(), std::format("You have gained a level!\n\n{}", level_effects_string));
	}

	const modifier<const metternich::character> *level_modifier = character_class->get_level_modifier(affected_level);
	if (level_modifier != nullptr) {
		level_modifier->apply(this->character);
	}

	const effect_list<const metternich::character> *effects = character_class->get_level_effects(affected_level);
	if (effects != nullptr) {
		context ctx(this->character);
		effects->do_effects(this->character, ctx);
	}
}

void character_game_data::check_level_experience()
{
	const metternich::character_class *character_class = this->get_character_class();
	if (character_class == nullptr) {
		return;
	}

	while (this->get_experience() >= this->get_experience_for_level(this->get_level() + 1)) {
		if (this->get_level() == character_class->get_max_level()) {
			break;
		}

		this->change_level(1);
	}
}

void character_game_data::change_experience(const int64_t change)
{
	if (change == 0) {
		return;
	}

	this->experience += change;

	if (game::get()->is_running()) {
		emit experience_changed();
	}

	this->check_level_experience();
}

int64_t character_game_data::get_experience_for_level(const int level) const
{
	assert_throw(this->get_character_class() != nullptr);
	int64_t experience = this->get_character_class()->get_experience_for_level(level);

	const int level_limit = this->character->get_species()->get_character_class_level_limit(this->get_character_class());
	assert_throw(level_limit > 0);
	if (level > level_limit) {
		//multiply experience required by 4 for levels beyond the species level limit for the class
		experience *= 4;
	}

	return experience;
}

int64_t character_game_data::get_experience_award() const
{
	if (this->character->get_monster_type() != nullptr) {
		return this->character->get_monster_type()->get_experience_award();
	}

	//FIXME: award experience for defeating characters without a monster type
	return 0;
}

void character_game_data::change_attribute_value(const character_attribute *attribute, const int change)
{
	if (change == 0) {
		return;
	}

	const int old_value = this->get_attribute_value(attribute);

	if (this->get_office() != nullptr) {
		this->apply_office_modifier(this->domain, this->get_office(), -1);
	}

	const int new_value = (this->attribute_values[attribute] += change);

	if (new_value == 0) {
		this->attribute_values.erase(attribute);
	}

	if (this->get_office() != nullptr) {
		this->apply_office_modifier(this->domain, this->get_office(), 1);
	}

	if (change > 0) {
		for (int i = old_value + 1; i <= new_value; ++i) {
			const modifier<const metternich::character> *value_modifier = attribute->get_value_modifier(i);
			if (value_modifier != nullptr) {
				value_modifier->apply(this->character);
			}
		}
	} else {
		for (int i = old_value; i > new_value; --i) {
			const modifier<const metternich::character> *value_modifier = attribute->get_value_modifier(i);
			if (value_modifier != nullptr) {
				value_modifier->remove(this->character);
			}
		}
	}
}

int character_game_data::get_primary_attribute_value() const
{
	assert_throw(this->get_character_class() != nullptr);

	return this->get_attribute_value(this->character->get_primary_attribute());
}

data_entry_set<character_attribute> character_game_data::get_main_attributes() const
{
	data_entry_set<character_attribute> attributes;

	if (this->character->get_primary_attribute() != nullptr) {
		attributes.insert(this->character->get_primary_attribute());
	}

	for (const character_trait *trait : this->get_traits()) {
		if (trait->get_attribute() != nullptr) {
			attributes.insert(trait->get_attribute());
		}
	}

	return attributes;
}

void character_game_data::apply_hit_dice(const dice &hit_dice)
{
	this->change_hit_dice_count(hit_dice.get_count());

	const int hit_point_increase = random::get()->roll_dice(hit_dice);
	this->change_max_hit_points(hit_point_increase);
	this->change_hit_points(hit_point_increase);
}

void character_game_data::set_hit_points(int hit_points)
{
	hit_points = std::min(hit_points, this->get_max_hit_points());

	if (hit_points == this->get_hit_points()) {
		return;
	}

	this->hit_points = hit_points;

	if (this->get_hit_points() < 0 && this->get_max_hit_points() > 0) {
		this->die();
	}

	if (game::get()->is_running()) {
		emit hit_points_changed();
	}
}

void character_game_data::change_hit_points(const int change)
{
	this->set_hit_points(this->get_hit_points() + change);
}

void character_game_data::set_max_hit_points(const int hit_points)
{
	if (hit_points == this->get_max_hit_points()) {
		return;
	}

	this->max_hit_points = hit_points;

	if (this->get_hit_points() > this->get_max_hit_points()) {
		this->set_hit_points(this->get_max_hit_points());
	}

	if (game::get()->is_running()) {
		emit max_hit_points_changed();
	}
}

void character_game_data::change_max_hit_points(const int change)
{
	this->set_max_hit_points(this->get_max_hit_points() + change);
}

void character_game_data::set_armor_class_bonus(const int bonus)
{
	if (bonus == this->get_armor_class_bonus()) {
		return;
	}

	this->armor_class_bonus = bonus;

	if (game::get()->is_running()) {
		emit armor_class_bonus_changed();
	}
}

void character_game_data::change_armor_class_bonus(const int change)
{
	this->set_armor_class_bonus(this->get_armor_class_bonus() + change);
}

void character_game_data::change_species_armor_class_bonus(const species *species, const int change)
{
	if (change == 0) {
		return;
	}

	const int new_value = (this->species_armor_class_bonuses[species] += change);

	if (new_value == 0) {
		this->species_armor_class_bonuses.erase(species);
	}

	if (game::get()->is_running()) {
		emit species_armor_class_bonuses_changed();
	}
}

void character_game_data::set_to_hit_bonus(const int bonus)
{
	if (bonus == this->get_to_hit_bonus()) {
		return;
	}

	this->to_hit_bonus = bonus;

	if (game::get()->is_running()) {
		emit to_hit_bonus_changed();
	}
}

void character_game_data::change_to_hit_bonus(const int change)
{
	this->set_to_hit_bonus(this->get_to_hit_bonus() + change);
}

QVariantList character_game_data::get_saving_throw_bonuses_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_saving_throw_bonuses());
}

void character_game_data::change_saving_throw_bonus(const saving_throw_type *type, const int change)
{
	if (change == 0) {
		return;
	}

	const int new_value = (this->saving_throw_bonuses[type] += change);
	if (new_value == 0) {
		this->saving_throw_bonuses.erase(type);
	}

	if (game::get()->is_running()) {
		emit saving_throw_bonuses_changed();
	}
}

bool character_game_data::is_skill_trained(const skill *skill) const
{
	return this->skill_trainings.contains(skill);
}

void character_game_data::change_skill_training(const skill *skill, const int change)
{
	if (change == 0) {
		return;
	}

	const int new_value = (this->skill_trainings[skill] += change);
	assert_throw(new_value >= 0);
	if (new_value == 0) {
		this->skill_trainings.erase(skill);
	}

	if (game::get()->is_running()) {
		emit skill_trainings_changed();
	}
}

void character_game_data::change_skill_value(const skill *skill, const int change)
{
	if (change == 0) {
		return;
	}

	const int new_value = (this->skill_values[skill] += change);
	if (new_value == 0) {
		this->skill_values.erase(skill);
	}

	if (game::get()->is_running()) {
		emit skill_values_changed();
	}
}

const dice &character_game_data::get_damage_dice() const
{
	for (const auto &[slot, items] : this->equipped_items) {
		if (!slot->is_weapon()) {
			continue;
		}

		assert_throw(!items.empty());
		return items.at(0)->get_type()->get_damage_dice();
	}

	if (this->character->get_monster_type() != nullptr) {
		return this->character->get_monster_type()->get_damage_dice();
	}

	static constexpr dice null_dice;
	return null_dice;
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
		if (this->get_trait_count_for_type(trait_type) >= defines::get()->get_max_character_traits_for_type(trait_type)) {
			return false;
		}
	}

	// characters cannot gain a trait which would reduce their main attributes below 1
	for (const character_attribute *attribute : this->get_main_attributes()) {
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
		assert_throw(this->get_domain() != nullptr);

		if (trait->get_office_modifier(this->get_office()) != nullptr || trait->get_scaled_office_modifier(this->get_office()) != nullptr) {
			this->apply_trait_office_modifier(trait, this->get_domain(), this->get_office(), multiplier);
		}
	}

	for (const auto &[attribute, bonus] : trait->get_attribute_bonuses()) {
		this->change_attribute_value(attribute, bonus * multiplier);
	}

	if (trait->get_modifier() != nullptr) {
		this->apply_modifier(trait->get_modifier(), multiplier);
	}

	if (trait->get_military_unit_modifier() != nullptr && this->get_military_unit() != nullptr) {
		this->apply_military_unit_modifier(this->get_military_unit(), multiplier);
	}
}

bool character_game_data::generate_trait(const character_trait_type trait_type, const character_attribute *target_attribute, const int target_attribute_bonus)
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

		if (target_attribute != nullptr) {
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
	const character_attribute *target_attribute = this->character->get_skill() != 0 ? this->character->get_primary_attribute() : nullptr;
	const int target_attribute_value = target_attribute ? this->character->get_skill() : 0;
	const int target_attribute_bonus = target_attribute ? (target_attribute_value - this->get_attribute_value(target_attribute)) : 0;

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
	return this->get_domain() != nullptr && this->get_domain()->get_government()->get_ruler() == this->character;
}

void character_game_data::set_office(const metternich::office *office)
{
	if (office == this->get_office()) {
		return;
	}

	if (office != nullptr) {
		assert_throw(!this->is_dead());
	}

	this->office = office;

	if (game::get()->is_running()) {
		emit office_changed();
	}
}

std::string character_game_data::get_office_modifier_string(const metternich::domain *domain, const metternich::office *office) const
{
	assert_throw(office != nullptr);

	std::string str;

	for (const character_trait *trait : this->get_traits()) {
		if (trait->get_office_modifier(office) == nullptr && trait->get_scaled_office_modifier(office) == nullptr) {
			continue;
		}

		const size_t indent = trait->has_hidden_name() ? 0 : 1;

		std::string trait_modifier_str;

		if (trait->get_office_modifier(office) != nullptr) {
			trait_modifier_str = trait->get_office_modifier(office)->get_string(domain, 1, indent);
		}

		if (trait->get_scaled_office_modifier(office) != nullptr) {
			trait_modifier_str = trait->get_scaled_office_modifier(office)->get_string(domain, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()), indent);
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

void character_game_data::apply_office_modifier(const metternich::domain *domain, const metternich::office *office, const int multiplier) const
{
	assert_throw(domain != nullptr);
	assert_throw(office != nullptr);

	for (const character_trait *trait : this->get_traits()) {
		this->apply_trait_office_modifier(trait, domain, office, multiplier);
	}
}

void character_game_data::apply_trait_office_modifier(const character_trait *trait, const metternich::domain *domain, const metternich::office *office, const int multiplier) const
{
	if (trait->get_office_modifier(office) != nullptr) {
		trait->get_office_modifier(office)->apply(domain, multiplier);
	}

	if (trait->get_scaled_office_modifier(office) != nullptr) {
		trait->get_scaled_office_modifier(office)->apply(domain, std::min(this->get_attribute_value(trait->get_attribute()), trait->get_max_scaling()) * multiplier);
	}
}

bool character_game_data::is_deployable() const
{
	if (this->character->get_military_unit_category() == military_unit_category::none) {
		return false;
	}

	return true;
}

void character_game_data::deploy_to_province(const metternich::domain *domain, const province *province)
{
	assert_throw(domain != nullptr);
	assert_throw(domain == this->get_domain());
	assert_throw(province != nullptr);
	assert_throw(!this->is_deployed());
	assert_throw(this->is_deployable());

	const military_unit_type *military_unit_type = domain->get_military()->get_best_military_unit_category_type(this->character->get_military_unit_category(), this->character->get_culture());

	auto military_unit = make_qunique<metternich::military_unit>(military_unit_type, domain, this->character);

	assert_throw(military_unit->can_move_to(province));

	military_unit->set_province(province);

	domain->get_military()->add_military_unit(std::move(military_unit));

	assert_throw(this->get_domain() != nullptr);
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

QVariantList character_game_data::get_items_qvariant_list() const
{
	return container::to_qvariant_list(this->get_items());
}

void character_game_data::add_item(qunique_ptr<item> &&item)
{
	metternich::item *item_ptr = item.get();
	this->items.push_back(std::move(item));
	emit items_changed();

	if (this->can_equip_item(item_ptr, false)) {
		this->equip_item(item_ptr);
	}
}

void character_game_data::remove_item(item *item)
{
	if (item->is_equipped()) {
		this->deequip_item(item);
	}

	vector::remove(this->items, item);
	emit items_changed();
}

bool character_game_data::can_equip_item(const item *item, const bool ignore_already_equipped) const
{
	const item_slot *slot = item->get_slot();
	if (slot == nullptr) {
		return false;
	}

	const int item_slot_count = this->character->get_species()->get_item_slot_count(slot);
	if (ignore_already_equipped) {
		if (item_slot_count == 0) {
			return false;
		}
	} else {
		if (item_slot_count == this->get_equipped_item_count(slot)) {
			return false;
		}

		if (item->get_type()->is_two_handed()) {
			for (const auto &[other_slot, other_items] : this->equipped_items) {
				assert_throw(!other_items.empty());

				if (other_slot->is_off_hand()) {
					//cannot equip a two-handed weapon if something is already equipped in the off-hand slot
					return false;
				}
			}
		} else if (slot->is_off_hand()) {
			for (const auto &[other_slot, other_items] : this->equipped_items) {
				for (const metternich::item *other_item : other_items) {
					if (other_item->get_type()->is_two_handed()) {
						//cannot equip something in the off-hand slot if a two-handed weapon is already equipped
						assert_throw(other_slot->is_weapon());
						return false;
					}
				}
			}
		}
	}

	return true;
}

void character_game_data::equip_item(item *item)
{
	assert_throw(item->get_slot() != nullptr);

	const int slot_count = this->character->get_species()->get_item_slot_count(item->get_slot());
	assert_throw(slot_count > 0);

	std::vector<metternich::item *> items_to_deequip;

	const int used_slots = this->get_equipped_item_count(item->get_slot());
	assert_throw(used_slots <= slot_count);
	if (used_slots == slot_count) {
		items_to_deequip.push_back(this->get_equipped_items(item->get_slot()).at(0));
	}

	if (item->get_type()->is_two_handed()) {
		for (const auto &[other_slot, other_items] : this->equipped_items) {
			if (other_slot->is_off_hand()) {
				//de-equip what is already equipped in the off-hand slot if a two-handed weapon is being equipped
				vector::merge(items_to_deequip, other_items);
			}
		}
	} else if (item->get_slot()->is_off_hand()) {
		for (const auto &[other_slot, other_items] : this->equipped_items) {
			for (metternich::item *other_item : other_items) {
				if (other_item->get_type()->is_two_handed()) {
					//de-equip two-handed weapons if something is being equipped in the off-hand slot
					assert_throw(other_slot->is_weapon());
					items_to_deequip.push_back(other_item);
				}
			}
		}
	}

	for (metternich::item *item_to_deequip : items_to_deequip) {
		this->deequip_item(item_to_deequip);
	}

	this->equipped_items[item->get_slot()].push_back(item);

	item->set_equipped(true);

	this->on_item_equipped(item, 1);

	if (game::get()->is_running()) {
		emit equipped_items_changed();
	}
}

void character_game_data::deequip_item(item *item)
{
	std::vector<metternich::item *> &equipped_items = this->equipped_items[item->get_slot()];
	std::erase(equipped_items, item);

	if (equipped_items.empty()) {
		this->equipped_items.erase(item->get_slot());
	}

	item->set_equipped(false);

	this->on_item_equipped(item, -1);

	if (game::get()->is_running()) {
		emit equipped_items_changed();
	}
}

void character_game_data::on_item_equipped(const item *item, const int multiplier)
{
	const item_type *type = item->get_type();
	if (type->get_modifier() != nullptr) {
		type->get_modifier()->apply(this->character, multiplier);
	}

	if (item->get_enchantment() != nullptr) {
		this->on_item_equipped_with_enchantment(item->get_enchantment(), multiplier);
	}
}

void character_game_data::on_item_equipped_with_enchantment(const enchantment *enchantment, const int multiplier)
{
	if (enchantment->get_modifier() != nullptr) {
		enchantment->get_modifier()->apply(this->character, multiplier);
	}

	for (const metternich::enchantment *subenchantment : enchantment->get_subenchantments()) {
		this->on_item_equipped_with_enchantment(subenchantment, multiplier);
	}
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
