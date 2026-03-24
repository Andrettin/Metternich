#include "metternich.h"

#include "character/character_game_data.h"

#include "character/age_category.h"
#include "character/bloodline.h"
#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_history.h"
#include "character/character_modifier_type.h"
#include "character/level_bonus_table.h"
#include "character/monster_type.h"
#include "character/mythic_path.h"
#include "character/profession_profitability.h"
#include "character/saving_throw_type.h"
#include "character/skill.h"
#include "character/status_effect.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/country_economy.h"
#include "domain/country_military.h"
#include "domain/country_technology.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "domain/domain_tier.h"
#include "domain/government_type.h"
#include "domain/office.h"
#include "engine_interface.h"
#include "game/character_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "infrastructure/building_item_slot.h"
#include "item/enchantment.h"
#include "item/item.h"
#include "item/item_class.h"
#include "item/item_slot.h"
#include "item/item_type.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "religion/deity.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/factor.h"
#include "script/flag.h"
#include "script/modifier.h"
#include "script/scripted_character_modifier.h"
#include "species/species.h"
#include "spell/spell.h"
#include "ui/icon.h"
#include "ui/portrait.h"
#include "unit/civilian_unit.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/date_util.h"
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
	connect(this, &character_game_data::office_changed, this, &character_game_data::full_name_changed);
	connect(this, &character_game_data::office_changed, this, &character_game_data::titled_name_changed);

	this->portrait = this->character->get_portrait();
	this->birth_date = this->character->get_birth_date();
	this->death_date = this->character->get_death_date();
	this->start_date = this->character->get_start_date();
	this->home_site = this->character->get_home_site();

	for (const skill *skill : skill::get_all()) {
		this->change_skill_value(skill, skill->get_base_value());
	}
}

void character_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "portrait") {
		this->portrait = portrait::get(value);
	} else if (key == "icon") {
		this->icon = icon::get(value);
	} else if (key == "domain") {
		this->domain = domain::get(value);
	} else if (key == "dead") {
		this->dead = string::to_bool(value);
	} else if (key == "birth_date") {
		this->birth_date = string::to_date(value);
	} else if (key == "death_date") {
		this->death_date = string::to_date(value);
	} else if (key == "start_date") {
		this->start_date = string::to_date(value);
	} else if (key == "home_site") {
		this->home_site = site::get(value);
	} else if (key == "character_class") {
		this->character_class = character_class::get(value);
	} else if (key == "level") {
		this->level = std::stoi(value);
	} else if (key == "experience") {
		this->experience = std::stoll(value);
	} else if (key == "challenge_rating") {
		this->challenge_rating = std::stoi(value);
	} else if (key == "bloodline") {
		this->bloodline = bloodline::get(value);
	} else if (key == "bloodline_strength") {
		this->bloodline_strength = std::stoi(value);
	} else if (key == "reputation") {
		this->reputation = std::stoi(value);
	} else if (key == "hit_dice_count") {
		this->hit_dice_count = std::stoi(value);
	} else if (key == "hit_points") {
		this->hit_points = std::stoi(value);
	} else if (key == "max_hit_points") {
		this->max_hit_points = std::stoi(value);
	} else if (key == "hit_point_bonus_per_hit_dice") {
		this->hit_point_bonus_per_hit_dice = std::stoi(value);
	} else if (key == "armor_class_bonus") {
		this->armor_class_bonus = std::stoi(value);
	} else if (key == "to_hit_bonus") {
		this->to_hit_bonus = std::stoi(value);
	} else if (key == "damage_bonus") {
		this->damage_bonus = std::stoi(value);
	} else if (key == "movement") {
		this->movement = std::stoi(value);
	} else if (key == "range") {
		this->range = std::stoi(value);
	} else {
		throw std::runtime_error(std::format("Invalid character game data property: \"{}\".", key));
	}
}

void character_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "attributes") {
		scope.for_each_property([this](const gsml_property &attribute_property) {
			this->attribute_values[character_attribute::get(attribute_property.get_key())] = std::stoi(attribute_property.get_value());
		});
	} else if (tag == "attributes_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();

			const character_attribute *attribute = character_attribute::get(child_tag);

			child_scope.for_each_child([this, attribute](const gsml_data &grandchild_scope) {
				const std::string &grandchild_tag = grandchild_scope.get_tag();
				const std::vector<std::string> &grandchild_values = grandchild_scope.get_values();

				const character_modifier_type modifier_type = magic_enum::enum_cast<character_modifier_type>(grandchild_tag).value();
				for (const std::string &grandchild_value : grandchild_values) {
					this->attribute_modifiers[attribute][modifier_type].push_back(std::stoi(grandchild_value));
				}
			});
		});
	} else if (tag == "hit_dice_roll_results") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const std::vector<std::string> &child_values = child_scope.get_values();

			const dice hit_dice(child_tag);
			for (const std::string &child_value : child_values) {
				this->hit_dice_roll_results[hit_dice].push_back(std::stoi(child_value));
			}
		});
	} else if (tag == "species_armor_class_bonuses") {
		scope.for_each_property([this](const gsml_property &property) {
			this->species_armor_class_bonuses[species::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "saving_throw_bonuses") {
		scope.for_each_property([this](const gsml_property &property) {
			this->saving_throw_bonuses[saving_throw_type::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "skill_trainings") {
		scope.for_each_property([this](const gsml_property &property) {
			this->skill_trainings[skill::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "skill_values") {
		scope.for_each_property([this](const gsml_property &property) {
			this->skill_values[skill::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "trait_counts") {
		scope.for_each_property([this](const gsml_property &property) {
			this->trait_counts[trait::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "trait_choices") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const std::vector<std::string> &child_values = child_scope.get_values();

			const trait_type *trait_type = trait_type::get(child_tag);
			for (const std::string &child_value : child_values) {
				this->trait_choices[trait_type].push_back(trait::get(child_value));
			}
		});
	} else if (tag == "spells") {
		for (const std::string &value : values) {
			this->spells.push_back(spell::get(value));
		}
	} else if (tag == "items") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			auto item = make_qunique<metternich::item>(child_scope);
			child_scope.process(item.get());
			metternich::item *item_ptr = item.get();
			this->items.push_back(std::move(item));

			if (item_ptr->is_equipped()) {
				this->equipped_items[item_ptr->get_slot()].push_back(item_ptr);
			}
		});
	} else if (tag == "ruled_domains") {
		for (const std::string &value : values) {
			this->ruled_domains.insert(domain::get(value));
		}
	} else if (tag == "reigned_domains") {
		for (const std::string &value : values) {
			this->reigned_domains.insert(domain::get(value));
		}
	} else if (tag == "flags") {
		for (const std::string &value : values) {
			this->flags.insert(flag::get(value));
		}
	} else {
		throw std::runtime_error(std::format("Invalid character game data scope: \"{}\".", tag));
	}
}

gsml_data character_game_data::to_gsml_data() const
{
	gsml_data data(this->character->get_identifier());

	data.add_property("portrait", this->get_portrait()->get_identifier());
	data.add_property("icon", this->get_icon()->get_identifier());

	if (this->get_domain() != nullptr) {
		data.add_property("domain", this->get_domain()->get_identifier());
	}

	data.add_property("dead", string::from_bool(this->is_dead()));
	data.add_property("start_date", date::to_string(this->get_start_date()));
	data.add_property("birth_date", date::to_string(this->get_birth_date()));
	data.add_property("death_date", date::to_string(this->get_death_date()));
	if (this->get_home_site() != nullptr) {
		data.add_property("home_site", this->get_home_site()->get_identifier());
	}
	if (this->get_character_class() != nullptr) {
		data.add_property("character_class", this->get_character_class()->get_identifier());
	}
	data.add_property("level", std::to_string(this->get_level()));
	data.add_property("experience", std::to_string(this->get_experience()));
	data.add_property("challenge_rating", std::to_string(this->get_challenge_rating()));
	if (this->get_bloodline() != nullptr) {
		data.add_property("bloodline", this->get_bloodline()->get_identifier());
		data.add_property("bloodline_strength", std::to_string(this->get_bloodline_strength()));
	}
	data.add_property("reputation", std::to_string(this->get_reputation()));

	if (!this->attribute_values.empty()) {
		gsml_data attributes_data("attributes");
		for (const auto &[attribute, value] : this->attribute_values) {
			attributes_data.add_property(attribute->get_identifier(), std::to_string(value));
		}
		data.add_child(std::move(attributes_data));
	}

	if (!this->attribute_modifiers.empty()) {
		gsml_data attribute_modifiers_data("attribute_modifiers");

		for (const auto &[attribute, specific_attribute_modifiers] : this->attribute_modifiers) {
			gsml_data specific_attribute_modifiers_data(attribute->get_identifier());

			for (const auto &[modifier_type, modifiers] : specific_attribute_modifiers) {
				gsml_data modifiers_data(std::string(magic_enum::enum_name(modifier_type)));

				for (const int modifier : modifiers) {
					modifiers_data.add_value(std::to_string(modifier));
				}

				specific_attribute_modifiers_data.add_child(std::move(modifiers_data));
			}

			attribute_modifiers_data.add_child(std::move(specific_attribute_modifiers_data));
		}

		data.add_child(std::move(attribute_modifiers_data));
	}

	data.add_property("hit_dice_count", std::to_string(this->get_hit_dice_count()));
	data.add_property("hit_points", std::to_string(this->get_hit_points()));
	data.add_property("max_hit_points", std::to_string(this->get_max_hit_points()));
	data.add_property("hit_point_bonus_per_hit_dice", std::to_string(this->get_hit_point_bonus_per_hit_dice()));
	data.add_property("armor_class_bonus", std::to_string(this->get_armor_class_bonus()));
	data.add_property("to_hit_bonus", std::to_string(this->get_to_hit_bonus()));
	data.add_property("damage_bonus", std::to_string(this->get_damage_bonus()));
	data.add_property("movement", std::to_string(this->get_movement()));
	data.add_property("range", std::to_string(this->get_range()));

	if (!this->hit_dice_roll_results.empty()) {
		gsml_data hit_dice_roll_results_data("hit_dice_roll_results");
		for (const auto &[hit_dice, roll_results] : this->hit_dice_roll_results) {
			gsml_data hit_dice_data(hit_dice.to_string());
			for (const int roll_result : roll_results) {
				hit_dice_data.add_value(std::to_string(roll_result));
			}
			hit_dice_roll_results_data.add_child(std::move(hit_dice_data));
		}
		data.add_child(std::move(hit_dice_roll_results_data));
	}

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

	if (!this->trait_counts.empty()) {
		gsml_data trait_counts_data("trait_counts");
		for (const auto &[trait, count] : this->trait_counts) {
			trait_counts_data.add_property(trait->get_identifier(), std::to_string(count));
		}
		data.add_child(std::move(trait_counts_data));
	}

	if (!this->trait_choices.empty()) {
		gsml_data trait_choices_data("trait_choices");
		for (const auto &[trait_type, traits] : this->trait_choices) {
			gsml_data trait_type_data(trait_type->get_identifier());
			for (const trait *trait : traits) {
				trait_type_data.add_value(trait->get_identifier());
			}
			trait_choices_data.add_child(std::move(trait_type_data));
		}
		data.add_child(std::move(trait_choices_data));
	}

	if (!this->spells.empty()) {
		gsml_data spells_data("spells");
		for (const spell *spell : this->spells) {
			spells_data.add_value(spell->get_identifier());
		}
		data.add_child(std::move(spells_data));
	}

	if (!this->items.empty()) {
		gsml_data items_data("items");
		for (const qunique_ptr<item> &item : this->items) {
			items_data.add_child(item->to_gsml_data());
		}
		data.add_child(std::move(items_data));
	}

	if (!this->get_ruled_domains().empty()) {
		gsml_data ruled_domains_data("ruled_domains");
		for (const metternich::domain *domain : this->get_ruled_domains()) {
			ruled_domains_data.add_value(domain->get_identifier());
		}
		data.add_child(std::move(ruled_domains_data));
	}
	if (!this->get_reigned_domains().empty()) {
		gsml_data reigned_domains_data("reigned_domains");
		for (const metternich::domain *domain : this->get_reigned_domains()) {
			reigned_domains_data.add_value(domain->get_identifier());
		}
		data.add_child(std::move(reigned_domains_data));
	}

	if (!this->flags.empty()) {
		gsml_data flags_data("flags");
		for (const flag *flag : this->flags) {
			flags_data.add_value(flag->get_identifier());
		}
		data.add_child(std::move(flags_data));
	}

	return data;
}

void character_game_data::ply_trade()
{
	const skill *best_skill = nullptr;
	profession_profitability skill_profitability = profession_profitability::none;
	int skill_percent_value = 0;

	for (const auto &[skill, value] : this->get_skill_values()) {
		if (!this->is_skill_trained(skill)) {
			continue;
		}

		if (skill->get_profitability() == profession_profitability::none) {
			continue;
		}

		if (value <= 0) {
			continue;
		}

		const int percent_value = value * 100 / skill->get_check_dice().get_sides();
		if (skill->get_profitability() > skill_profitability || (skill->get_profitability() == skill_profitability && percent_value > skill_percent_value)) {
			best_skill = skill;
			skill_profitability = skill->get_profitability();
			skill_percent_value = percent_value;
		}
	}

	if (best_skill == nullptr) {
		return;
	}

	if (!this->do_skill_check(best_skill, 0, this->get_location())) {
		return;
	}

	const int province_level = this->get_location()->get_game_data()->get_province()->get_game_data()->get_level();
	if (province_level <= 3 && skill_profitability != profession_profitability::marginal) {
		skill_profitability = static_cast<profession_profitability>(static_cast<int>(skill_profitability) - 1);
	} else if (province_level >= 7 && skill_profitability != profession_profitability::excellent) {
		skill_profitability = static_cast<profession_profitability>(static_cast<int>(skill_profitability) + 1);
	}

	int profit = 0;

	static constexpr int gp_value = 100;

	switch (skill_profitability) {
		case profession_profitability::marginal:
		{
			static constexpr dice profit_dice(5, 6);
			profit = random::get()->roll_dice(profit_dice) * gp_value;
			break;
		}
		case profession_profitability::fair:
		{
			static constexpr dice profit_dice(10, 6);
			profit = random::get()->roll_dice(profit_dice) * gp_value;
			break;
		}
		case profession_profitability::good:
		{
			static constexpr dice profit_dice(1, 6, 2);
			profit = random::get()->roll_dice(profit_dice) * gp_value * 10;
			break;
		}
		case profession_profitability::excellent:
		{
			static constexpr dice profit_dice(6, 4);
			profit = random::get()->roll_dice(profit_dice) * gp_value * 10;
			break;
		}
		default:
			assert_throw(false);
			break;
	}

	this->change_wealth(profit);
}

void character_game_data::do_events()
{
	const bool is_last_turn_of_year = game::get()->is_last_turn_of_year();
	if (is_last_turn_of_year) {
		character_event::check_events_for_scope(this->character, event_trigger::yearly_pulse);
	}

	const bool is_last_turn_of_quarter = game::get()->is_last_turn_of_quarter();
	if (is_last_turn_of_quarter) {
		character_event::check_events_for_scope(this->character, event_trigger::quarterly_pulse);
	}

	character_event::check_events_for_scope(this->character, event_trigger::per_turn_pulse);
}

bool character_game_data::is_ai() const
{
	return this->character != game::get()->get_player_character();
}

void character_game_data::apply_species_and_class(const int level, const bool apply_history)
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
		if (monster_type->get_modifier() != nullptr) {
			monster_type->get_modifier()->apply(this->character);
		}
	}

	this->generate_attributes();
	this->apply_bloodline(apply_history);
	if (this->get_reputation() < character::base_reputation) {
		this->set_reputation(character::base_reputation);
	}

	const metternich::character_class *character_class = this->get_character_class();
	if (character_class != nullptr) {
		this->set_level(std::min(level, character_class->get_max_level()));
	}

	const metternich::mythic_path *mythic_path = this->character->get_mythic_path();
	if (mythic_path != nullptr) {
		for (int i = 1; i <= this->character->get_mythic_tier(); ++i) {
			this->on_mythic_tier_gained(i, 1);
		}
	}

	if (this->is_deity()) {
		for (int i = 1; i <= this->character->get_deity()->get_divine_level(); ++i) {
			this->on_divine_rank_gained(i, 1);
		}

		int base_major_divine_domain_count = static_cast<int>(this->character->get_deity()->get_major_domains().size());

		for (const auto &[trait, count] : this->get_trait_counts()) {
			if (trait->get_divine_domain() != nullptr) {
				assert_throw(vector::contains(this->character->get_deity()->get_major_domains(), trait->get_divine_domain()));
				--base_major_divine_domain_count;
			}
		}

		if (base_major_divine_domain_count > deity::base_deity_major_domains) {
			const int major_domain_count = static_cast<int>(this->character->get_deity()->get_major_domains().size());
			log::log_error(std::format("Deity \"{}\" has {} major domains, but its base major domains and traits only provide access to {} of them.", this->character->get_deity()->get_identifier(), major_domain_count, major_domain_count - (base_major_divine_domain_count - deity::base_deity_major_domains)));
		}
	}

	if (character_class != nullptr) {
		while (!this->target_traits.empty()) {
			if (this->get_level() == character_class->get_max_level()) {
				break;
			}

			this->change_level(1);
		}
	}

	if (!this->target_traits.empty()) {
		throw std::runtime_error(std::format("Could not acquire all target traits for character \"{}\".", this->character->get_identifier()));
	}

	this->add_starting_items();
	this->add_starting_spells();

	if (this->character->get_hit_points() != 0) {
		int min_hp = 0;
		int max_hp = 0;

		for (const auto &[hit_dice, roll_results] : this->hit_dice_roll_results) {
			min_hp += std::max(hit_dice.get_minimum_result() + this->get_hit_point_bonus_per_hit_dice(), hit_dice.get_count()) * static_cast<int>(roll_results.size());
			max_hp += std::max(hit_dice.get_maximum_result() + this->get_hit_point_bonus_per_hit_dice(), hit_dice.get_count()) * static_cast<int>(roll_results.size());
		}

		assert_log(this->character->get_hit_points() >= min_hp);
		assert_log(this->character->get_hit_points() <= max_hp);

		this->set_max_hit_points(this->character->get_hit_points(), true);
	}

	//ensure characters start with their hit point maximum
	this->set_hit_points(this->get_max_hit_points());
}

void character_game_data::generate_attributes()
{
	const species *species = this->character->get_species();
	const metternich::character_class *character_class = this->get_character_class();

	for (const character_attribute *attribute : character_attribute::get_all()) {
		const std::optional<std::pair<int, int>> attribute_range = this->character->get_attribute_range(attribute);

		int min_result = 0;
		min_result = std::max(species->get_min_attribute_value(attribute), min_result);
		if (character_class != nullptr) {
			min_result = std::max(character_class->get_min_attribute_value(attribute), min_result);
		}

		int max_result = std::numeric_limits<int>::max();
		if (species->get_max_attribute_value(attribute) != 0) {
			max_result = std::min(species->get_max_attribute_value(attribute), max_result);
		}

		if (attribute_range.has_value()) {
			const int attribute_range_min = attribute_range.value().first;
			const int attribute_range_max = attribute_range.value().second;
			min_result = std::max(attribute_range_min, min_result);
			max_result = std::min(attribute_range_max, max_result);
		}

		assert_throw(max_result >= min_result);

		if (min_result == max_result) {
			this->change_attribute_value(attribute, min_result - this->get_attribute_value(attribute));
			continue;
		}

		static constexpr dice attribute_dice(3, 6);
		const int minimum_possible_result = attribute_dice.get_minimum_result() + this->get_attribute_value(attribute);
		const int maximum_possible_result = attribute_dice.get_maximum_result() + this->get_attribute_value(attribute);
		if ((maximum_possible_result < min_result || minimum_possible_result > max_result)) {
			throw std::runtime_error(std::format("Character \"{}\" of species \"{}\" cannot be generated{}, since it cannot possibly fulfill the attribute requirements.", this->character->get_identifier(), species->get_identifier(), character_class != nullptr ? std::format(" with character class \"{}\"", character_class->get_identifier()) : ""));
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
}

void character_game_data::apply_bloodline(const bool apply_history)
{
	assert_throw(this->get_bloodline() == nullptr);

	if (this->character->get_bloodline() != nullptr) {
		assert_throw(this->character->get_bloodline_strength() > 0);
		this->set_bloodline(this->character->get_bloodline());
		this->set_bloodline_strength(this->character->get_bloodline_strength());
		return;
	}

	this->apply_bloodline_from_parents();

	if (apply_history) {
		this->apply_bloodline_inheritance_investiture();
	}
}

void character_game_data::apply_bloodline_from_parents()
{
	assert_throw(this->get_bloodline() == nullptr);

	//if the character does not have a predefined bloodline and bloodline strength, see if it can inherit them from their parents
	const std::vector<metternich::character *> biological_parents = this->character->get_biological_parents();

	if (biological_parents.empty()) {
		return;
	}

	std::vector<const metternich::bloodline *> potential_bloodlines;
	int best_bloodline_strength = 0;
	int bloodline_strength = 0;

	for (const metternich::character *parent : biological_parents) {
		if (parent->get_game_data()->get_bloodline() == nullptr) {
			continue;
		}

		const int parent_bloodline_strength = parent->get_game_data()->get_bloodline_strength();
		bloodline_strength += parent_bloodline_strength;

		if (parent_bloodline_strength < best_bloodline_strength) {
			continue;
		}

		if (parent_bloodline_strength > best_bloodline_strength) {
			potential_bloodlines.clear();
			best_bloodline_strength = parent_bloodline_strength;
		}

		potential_bloodlines.push_back(parent->get_game_data()->get_bloodline());
	}

	if (potential_bloodlines.empty()) {
		return;
	}

	bloodline_strength /= 2;

	if (bloodline_strength > 0) {
		this->set_bloodline(vector::get_random(potential_bloodlines));
		this->set_bloodline_strength(bloodline_strength);
		log_trace(std::format("Set bloodline for character \"{}\": {} ({}).", this->character->get_identifier(), this->get_bloodline()->get_identifier(), bloodline_strength));
	}
}

void character_game_data::apply_bloodline_inheritance_investiture()
{
	//apply bloodline inheritance from predecessors (inheritance in the sense of inheriting a domain, not of genetic inheritance)

	//see if we can inherit the bloodline in full from any predecessor
	const std::vector<const metternich::character *> predecessors = this->character->get_history()->get_predecessors();

	if (predecessors.empty()) {
		return;
	}

	for (const metternich::character *predecessor : predecessors) {
		if (!predecessor->get_game_data()->is_dead()) {
			continue;
		}

		this->inherit_bloodline_from(predecessor);
	}
}

void character_game_data::add_starting_items()
{
	const metternich::character_class *character_class = this->get_character_class();

	data_entry_set<item_slot> filled_item_slots;

	if (!this->character->get_starting_items().empty()) {
		this->add_starting_items(this->character->get_starting_items(), filled_item_slots);
	}

	if (this->character->get_monster_type() != nullptr) {
		this->add_starting_items(this->character->get_monster_type()->get_items(), filled_item_slots);
	}

	if (character_class != nullptr) {
		this->add_starting_items(character_class->get_starting_items(), filled_item_slots);
	}
}

void character_game_data::add_starting_items(const std::vector<const item_type *> &starting_items, data_entry_set<item_slot> &filled_item_slots)
{
	if (starting_items.empty()) {
		return;
	}

	data_entry_set<item_slot> new_filled_item_slots = filled_item_slots;

	for (const item_type *starting_item_type : starting_items) {
		if (starting_item_type->get_slot() != nullptr && filled_item_slots.contains(starting_item_type->get_slot())) {
			continue;
		}

		auto item = make_qunique<metternich::item>(starting_item_type, nullptr, nullptr, nullptr);
		if (item->get_slot() != nullptr) {
			new_filled_item_slots.insert(item->get_slot());

			//if the item is a two-handed weapon, it should also add mark the shield slot as filled
			if (item->get_type()->is_two_handed()) {
				for (const item_slot *slot : item_slot::get_all()) {
					if (slot->is_off_hand()) {
						new_filled_item_slots.insert(slot);
					}
				}
			}
		}
		this->add_item(std::move(item));
	}

	filled_item_slots = new_filled_item_slots;
}

void character_game_data::add_starting_spells()
{
	const metternich::character_class *character_class = this->get_character_class();

	if (!this->character->get_starting_spells().empty()) {
		this->add_starting_spells(this->character->get_starting_spells());
	} else if (this->character->get_monster_type() != nullptr && !this->character->get_monster_type()->get_spells().empty()) {
		this->add_starting_spells(this->character->get_monster_type()->get_spells());
	} else if (character_class != nullptr) {
		this->add_starting_spells(character_class->get_starting_spells());
	}
}

void character_game_data::add_starting_spells(const std::vector<const spell *> &starting_spells)
{
	if (starting_spells.empty()) {
		return;
	}

	for (const spell *spell : starting_spells) {
		if (this->can_learn_spell(spell)) {
			this->add_spell(spell);
		}
	}
}

void character_game_data::apply_history(const QDate &start_date)
{
	if (this->get_death_date().isValid() && start_date >= this->get_death_date()) {
		this->set_dead(true);
	}

	const character_history *character_history = this->character->get_history();

	this->character_class = this->character->get_character_class();

	this->set_target_traits(character_history->get_traits());
	if (this->is_deity()) {
		vector::merge(this->target_traits, this->character->get_deity()->get_traits());
	}

	const int level = std::max(character_history->get_level(), 1);
	this->apply_species_and_class(level, true);

	if (start_date < this->get_start_date()) {
		return;
	}

	if (this->is_dead()) {
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

	if (this->get_domain() == nullptr && this->get_home_site() != nullptr) {
		this->set_domain(this->get_home_site()->get_game_data()->get_owner());
	}
}

void character_game_data::on_setup_finished()
{
	this->check_portrait();
	this->check_icon();

	if (this->get_home_site() != nullptr) {
		this->get_home_site()->get_game_data()->add_homed_character(this->character);
	}
}

std::string character_game_data::get_full_name() const
{
	if (this->is_deity()) {
		return this->character->get_deity()->get_name();
	}

	const std::optional<std::pair<const metternich::domain *, int>> regnal_domain_and_number = this->get_regnal_domain_and_number();
	if (regnal_domain_and_number.has_value()) {
		return this->character->get_full_name(regnal_domain_and_number.value().first, regnal_domain_and_number.value().second);
	} else {
		return this->character->get_full_name(nullptr, std::nullopt);
	}
}

QString character_game_data::get_full_name_for_domain(const metternich::domain *domain) const
{
	if (this->is_deity()) {
		return QString::fromStdString(this->character->get_deity()->get_cultural_name(domain->get_game_data()->get_culture()));
	}

	//we presume here that we don't want to show the domain itself in the name, hence why we pass nullptr
	return QString::fromStdString(this->character->get_full_name(nullptr, this->get_regnal_number_for_domain(domain)));
}

std::string character_game_data::get_titled_name() const
{
	if (this->get_office() != nullptr) {
		assert_throw(this->get_domain() != nullptr);
		return std::format("{} {}", this->get_domain()->get_government()->get_office_title_name(this->get_office()), this->get_full_name());
	}

	return this->get_full_name();
}

std::optional<std::pair<const domain *, int>> character_game_data::get_regnal_domain_and_number() const
{
	if (this->is_dead()) {
		return this->get_best_regnal_domain_and_number();
	}

	if (this->get_office() == nullptr) {
		return std::nullopt;
	}

	if (!this->get_office()->is_ruler()) {
		return std::nullopt;
	}

	assert_throw(this->get_domain() != nullptr);

	if (!this->get_domain()->get_game_data()->get_government_type()->has_regnal_numbering()) {
		return std::nullopt;
	}

	const std::optional<int> regnal_number = this->get_regnal_number_for_domain(this->get_domain());
	if (!regnal_number.has_value()) {
		return std::nullopt;
	}

	return std::make_pair(this->get_domain(), regnal_number.value());
}

std::optional<int> character_game_data::get_regnal_number_for_domain(const metternich::domain *domain) const
{
	assert_throw(domain != nullptr);

	int regnal_number = 0;

	const std::map<QDate, const metternich::character *> &historical_monarchs = domain->get_game_data()->get_historical_monarchs();
	for (const auto &[date, historical_monarch] : historical_monarchs) {
		if (historical_monarch == nullptr) {
			//a null historical monarch means the regnal numbering was cleared
			regnal_number = 0;
			continue;
		}

		if (historical_monarch->get_name() == this->character->get_name()) {
			++regnal_number;
		}

		if (historical_monarch == this->character) {
			break;
		}
	}

	if (regnal_number == 0) {
		return std::nullopt;
	}

	return regnal_number;
}

std::optional<std::pair<const domain *, int>> character_game_data::get_best_regnal_domain_and_number() const
{
	domain_tier best_min_tier = domain_tier::none;
	domain_tier best_max_tier = domain_tier::none;
	const metternich::domain *best_domain = nullptr;

	for (const metternich::domain *domain : this->get_reigned_domains()) {
		if (domain->get_max_tier() > best_max_tier) {
			best_domain = domain;
			best_min_tier = domain->get_min_tier();
			best_max_tier = domain->get_max_tier();
		} else if (domain->get_max_tier() == best_max_tier && domain->get_min_tier() >= best_min_tier) {
			best_domain = domain;
			best_min_tier = domain->get_min_tier();
			best_max_tier = domain->get_max_tier();
		}
	}

	if (best_domain == nullptr) {
		return std::nullopt;
	}

	const std::optional<int> regnal_number = this->get_regnal_number_for_domain(best_domain);
	if (!regnal_number.has_value()) {
		return std::nullopt;
	}

	return std::make_pair(best_domain, regnal_number.value());
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
	emit portrait_changed();
}

bool character_game_data::is_current_icon_valid() const
{
	if (this->get_icon() == nullptr) {
		return false;
	}

	assert_throw(this->get_icon()->get_character_conditions() != nullptr);

	if (!this->get_icon()->get_character_conditions()->check(this->character, read_only_context(this->character))) {
		return false;
	}

	return true;
}

void character_game_data::check_icon()
{
	if (this->is_current_icon_valid()) {
		return;
	}

	std::vector<const metternich::icon *> potential_icons;

	for (const metternich::icon *icon : icon::get_character_icons()) {
		assert_throw(icon->is_character_icon());
		assert_throw(icon->get_character_conditions() != nullptr);

		if (!icon->get_character_conditions()->check(this->character, read_only_context(this->character))) {
			continue;
		}

		potential_icons.push_back(icon);
	}

	//there must always be an available icon for characters
	if (potential_icons.empty()) {
		throw std::runtime_error(std::format("No icon is suitable for character \"{}\".", this->character->get_identifier()));
	}

	this->icon = vector::get_random(potential_icons);
	emit icon_changed();
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
	const QDate &birth_date = this->get_birth_date();
	const QDate &end_date = this->is_dead() ? this->get_death_date() : game::get()->get_date();

	int age = end_date.year() - birth_date.year() - 1;

	const QDate end_birthday(end_date.year(), birth_date.month(), birth_date.day());
	if (end_date >= end_birthday) {
		++age;
	}

	return age;
}

age_category character_game_data::get_age_category() const
{
	const int age = this->get_age();
	if (age >= this->character->get_species()->get_venerable_age()) {
		return age_category::venerable;
	} else if (age >= this->character->get_species()->get_old_age()) {
		return age_category::old;
	} else if (age >= this->character->get_species()->get_middle_age()) {
		return age_category::middle_aged;
	} else if (age >= this->character->get_species()->get_adulthood_age()) {
		return age_category::adult;
	} else {
		return age_category::child;
	}
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

	if (this->get_civilian_unit() != nullptr) {
		this->get_civilian_unit()->disband(true);
	}

	assert_throw(this->get_office() == nullptr);
	if (this->get_wealth() > 0 || !this->get_items().empty()) {
		character_set checked_characters;
		std::vector<const metternich::character *> next_of_kin = this->get_next_of_kin(checked_characters, true);

		if (next_of_kin.empty()) {
			//if there is no next of kin, the ruler of the character's domain inherits their personal wealth
			next_of_kin = { this->get_domain()->get_government()->get_ruler() };
		}

		if (!next_of_kin.empty()) {
			const int wealth_share = this->get_wealth() / static_cast<int>(next_of_kin.size());
			for (const metternich::character * inheritor : next_of_kin) {
				inheritor->get_game_data()->change_wealth(wealth_share);

				std::vector<item *> potential_items;
				for (const qunique_ptr<item> &item : this->get_items()) {
					potential_items.push_back(item.get());
				}
				for (item *item : potential_items) {
					if (item->is_useful_for(inheritor)) {
						inheritor->get_game_data()->add_item(this->take_item(item));
					}
				}
			}
		}

		this->wealth = 0;
	}

	assert_throw(this->get_office() == nullptr);
	this->set_domain(nullptr);

	if (this->is_deity() && this->character->get_deity()->is_apotheotic()) {
		for (int i = 1; i <= this->character->get_deity()->get_divine_level(); ++i) {
			this->on_divine_rank_gained(i, 1);
		}
	}
}

bool character_game_data::exists() const
{
	//whether the character exists in the game
	if (this->character->is_innate_deity()) {
		return true;
	}

	return this->get_domain() != nullptr;
}

bool character_game_data::has_ever_existed() const
{
	//whether the character exists in the game or the scenario history
	if (this->exists()) {
		return true;
	}

	//if (this->get_home_site() != nullptr && !this->get_home_site()->get_game_data()->is_on_map()) {
	//	return false;
	//}

	return this->is_dead();
}

bool character_game_data::is_playable() const
{
	if (this->get_character_class() == nullptr) {
		return false;
	}

	if (this->character->is_innate_deity()) {
		return false;
	}

	return true;
}

QString character_game_data::get_unplayable_reason() const
{
	if (this->get_character_class() == nullptr) {
		return "You cannot play as a character without a character class";
	}

	if (this->character->is_innate_deity()) {
		return "You cannot play as a deity";
	}

	return QString();
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

QVariantList character_game_data::get_children_qvariant_list() const
{
	return container::to_qvariant_list(this->get_children());
}

std::vector<const metternich::character *> character_game_data::get_dynastic_children() const
{
	std::vector<const metternich::character *> children;

	for (const metternich::character *child : this->get_children()) {
		if (child->get_dynastic_parent() == this->character) {
			children.push_back(child);
		}
	}

	return children;
}

QVariantList character_game_data::get_dynastic_children_qvariant_list() const
{
	return container::to_qvariant_list(this->get_dynastic_children());
}

std::vector<const metternich::character *> character_game_data::get_next_of_kin(character_set &checked_characters, const bool include_parents) const
{
	std::vector<const metternich::character *> next_of_kin;

	checked_characters.insert(this->character);

	//children are expected to be sorted by birth order
	const std::vector<const metternich::character *> children = this->get_children();

	for (const metternich::character *child : children) {
		if (checked_characters.contains(child)) {
			continue;
		}

		if (child->get_game_data()->exists()) {
			checked_characters.insert(child);
			next_of_kin.push_back(child);
			continue;
		}

		vector::merge(next_of_kin, child->get_game_data()->get_next_of_kin(checked_characters, false));
	}

	if (include_parents) {
		for (const metternich::character *parent : this->character->get_parents()) {
			if (checked_characters.contains(parent)) {
				continue;
			}

			if (parent->get_game_data()->exists()) {
				checked_characters.insert(parent);
				next_of_kin.push_back(parent);
				continue;
			}

			vector::merge(next_of_kin, parent->get_game_data()->get_next_of_kin(checked_characters, true));
		}
	}

	return next_of_kin;
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

	const std::variant<int, dice> &hit_point_bonus = character_class->get_hit_point_bonus_for_level(affected_level);
	if (std::holds_alternative<int>(hit_point_bonus)) {
		const int hit_point_bonus_int = std::get<int>(hit_point_bonus);
		this->change_max_hit_points(hit_point_bonus_int * multiplier, true);
	} else if (std::holds_alternative<dice>(hit_point_bonus)) {
		const dice &hit_point_bonus_dice = std::get<dice>(hit_point_bonus);

		if (multiplier > 0) {
			this->apply_hit_dice(hit_point_bonus_dice);
		} else if (multiplier < 0) {
			this->remove_hit_dice(hit_point_bonus_dice);
		}
	}

	if (character_class->get_to_hit_bonus_table() != nullptr) {
		this->change_to_hit_bonus(character_class->get_to_hit_bonus_table()->get_bonus_per_level(affected_level) * multiplier);
	}

	for (const saving_throw_type *saving_throw_type : saving_throw_type::get_all()) {
		const level_bonus_table *saving_throw_bonus_table = character_class->get_saving_throw_bonus_table(saving_throw_type);
		this->change_saving_throw_bonus(saving_throw_type, saving_throw_bonus_table->get_bonus_per_level(affected_level) * multiplier);
	}

	const modifier<const metternich::character> *level_modifier = character_class->get_level_modifier(affected_level);
	if (level_modifier != nullptr) {
		level_modifier->apply(this->character);
	}

	if (game::get()->is_running() && this->character == game::get()->get_player_character()) {
		const std::string level_modifier_string = character_class->get_level_modifier_string(affected_level, this->character);

		engine_interface::get()->add_notification("Level Up", this->get_portrait(), std::format("You have gained a level!\n\n{}", level_modifier_string));
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

		this->change_experience(-this->get_experience_for_level(this->get_level() + 1));
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
	if (level > 1) {
		experience -= this->get_character_class()->get_experience_for_level(level - 1);
	}

	const int level_limit = this->character->get_species()->get_character_class_level_limit(this->get_character_class());
	assert_throw(level_limit > 0);
	if (level > level_limit) {
		//multiply experience required by 4 for levels beyond the species level limit for the class
		experience *= 4;
	}

	return experience;
}

void character_game_data::change_challenge_rating(const int change)
{
	if (change == 0) {
		return;
	}

	this->challenge_rating += change;
}

int64_t character_game_data::get_experience_award() const
{
	if (this->character->get_monster_type() != nullptr && this->character->get_monster_type()->get_experience_award() != 0) {
		return this->character->get_monster_type()->get_experience_award();
	}

	return defines::get()->get_experience_award_for_challenge_rating(this->get_challenge_rating());
}

void character_game_data::on_mythic_tier_gained(const int affected_tier, const int multiplier)
{
	assert_throw(std::abs(multiplier) == 1);

	assert_throw(affected_tier >= 1);

	const metternich::mythic_path *mythic_path = this->character->get_mythic_path();
	assert_throw(mythic_path != nullptr);

	const modifier<const metternich::character> *tier_modifier = mythic_path->get_tier_modifier(affected_tier);
	if (tier_modifier != nullptr) {
		tier_modifier->apply(this->character, multiplier);
	}

	if (game::get()->is_running() && this->character == game::get()->get_player_character()) {
		const std::string tier_modifier_string = mythic_path->get_tier_modifier_string(affected_tier, this->character);

		engine_interface::get()->add_notification("Mythic Tier Gained", this->get_portrait(), std::format("You have gained a mythic tier!\n\n{}", tier_modifier_string));
	}
}

bool character_game_data::is_deity() const
{
	if (this->character->get_deity() == nullptr) {
		return false;
	}

	if (this->character->get_deity()->is_apotheotic()) {
		return this->is_dead();
	} else {
		return true;
	}
}

void character_game_data::on_divine_rank_gained(const int affected_rank, const int multiplier)
{
	assert_throw(std::abs(multiplier) == 1);

	assert_throw(affected_rank >= 1);

	const modifier<const metternich::character> *rank_modifier = defines::get()->get_divine_rank_modifier(affected_rank);
	if (rank_modifier != nullptr) {
		rank_modifier->apply(this->character, multiplier);
	}
}

void character_game_data::set_bloodline(const metternich::bloodline *bloodline)
{
	if (bloodline == this->get_bloodline()) {
		return;
	}

	this->bloodline = bloodline;

	if (game::get()->is_running()) {
		emit bloodline_changed();
	}
}

void character_game_data::set_bloodline_strength(const int bloodline_strength)
{
	if (bloodline_strength == this->get_bloodline_strength()) {
		return;
	}

	//the bloodline must always be set before the bloodline strength
	assert_throw(this->get_bloodline() != nullptr);

	const int change = bloodline_strength - this->get_bloodline_strength();

	this->bloodline_strength = bloodline_strength;

	//FIXME: apply bloodline effects

	this->change_reputation(change);

	if (game::get()->is_running()) {
		emit bloodline_strength_changed();
	}
}

void character_game_data::inherit_bloodline_from(const metternich::character *other_character)
{
	//inherit bloodline (via investiture) from another character

	if (other_character->get_game_data()->get_bloodline() == nullptr) {
		other_character;
	}

	if (other_character->get_game_data()->get_bloodline_strength() < this->get_bloodline_strength()) {
		return;
	}

	if (this->get_bloodline() == nullptr) {
		this->set_bloodline(other_character->get_game_data()->get_bloodline());
	}

	this->set_bloodline_strength(other_character->get_game_data()->get_bloodline_strength());
}

void character_game_data::set_reputation(const int reputation)
{
	if (reputation == this->get_reputation()) {
		return;
	}

	this->reputation = reputation;

	if (game::get()->is_running()) {
		emit reputation_changed();
	}
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

	for (const skill *skill : attribute->get_derived_skills()) {
		this->change_skill_value(skill, change);
	}

	if (this->get_office() != nullptr) {
		this->apply_office_modifier(this->domain, this->get_office(), 1);
	}

	this->on_attribute_value_changed(attribute, new_value, old_value);

	if (game::get()->is_running()) {
		emit attribute_values_changed();
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

	return attributes;
}

bool character_game_data::do_attribute_check(const character_attribute *attribute, const int roll_modifier) const
{
	static constexpr dice check_dice(1, 20);

	const int roll_result = random::get()->roll_dice(check_dice);

	//there should always be at least a 5% chance of failure
	if (check_dice.get_sides() == 100) {
		if (roll_result >= 96) {
			return false;
		}
	} else if (roll_result == check_dice.get_sides()) {
		//e.g. if a 20 is rolled for a d20 roll
		return false;
	}

	const int attribute_value = this->get_attribute_value(attribute);
	const int modified_attribute_value = attribute_value + roll_modifier;
	return roll_result <= modified_attribute_value;
}

int character_game_data::get_attribute_check_chance(const character_attribute *attribute, const int roll_modifier) const
{
	assert_throw(attribute != nullptr);

	int chance = this->get_attribute_value(attribute);
	chance += roll_modifier;

	static constexpr dice check_dice(1, 20);

	if (check_dice.get_sides() != 100) {
		chance *= 100;
		chance /= check_dice.get_sides();
	}

	chance = std::min(chance, 95);

	return chance;
}

void character_game_data::on_attribute_value_changed(const character_attribute_base *attribute, const int new_value, const int old_value)
{
	if (new_value > old_value) {
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

void character_game_data::apply_hit_dice(const dice &hit_dice)
{
	this->change_hit_dice_count(hit_dice.get_count());

	const int roll_result = std::max(random::get()->roll_dice(hit_dice), hit_dice.get_count());
	const int hit_point_increase = std::max(roll_result + this->get_hit_point_bonus_per_hit_dice(), hit_dice.get_count());

	this->change_max_hit_points(hit_point_increase, true);

	this->hit_dice_roll_results[hit_dice].push_back(roll_result);
}

void character_game_data::remove_hit_dice(const dice &hit_dice)
{
	this->change_hit_dice_count(-hit_dice.get_count());

	const auto find_iterator = this->hit_dice_roll_results.find(hit_dice);
	assert_throw(find_iterator != this->hit_dice_roll_results.end());

	std::vector<int> &roll_results = find_iterator->second;
	assert_throw(!roll_results.empty());
	
	const int last_roll_result = roll_results.back();
	const int last_hit_point_increase = std::max(last_roll_result + this->get_hit_point_bonus_per_hit_dice(), hit_dice.get_count());
	this->change_max_hit_points(-last_hit_point_increase, false);

	roll_results.pop_back();
	if (roll_results.empty()) {
		this->hit_dice_roll_results.erase(hit_dice);
	}
}

void character_game_data::set_hit_points(int hit_points)
{
	hit_points = std::min(hit_points, this->get_max_hit_points());

	if (hit_points == this->get_hit_points()) {
		return;
	}

	this->hit_points = hit_points;

	if (this->get_hit_points() <= 0 && this->get_max_hit_points() > 0) {
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

void character_game_data::set_max_hit_points(const int max_hit_points, const bool increase_hit_points)
{
	if (max_hit_points == this->get_max_hit_points()) {
		return;
	}

	const int change = max_hit_points - this->get_max_hit_points();

	this->max_hit_points = max_hit_points;

	if (this->get_hit_points() > this->get_max_hit_points()) {
		this->set_hit_points(this->get_max_hit_points());
	} else if (change > 0 && increase_hit_points) {
		this->change_hit_points(change);
	}

	if (game::get()->is_running()) {
		emit max_hit_points_changed();
	}
}

void character_game_data::change_max_hit_points(const int change, const bool increase_hit_points)
{
	this->set_max_hit_points(this->get_max_hit_points() + change, increase_hit_points);
}

void character_game_data::set_hit_point_bonus_per_hit_dice(const int bonus)
{
	if (bonus == this->get_hit_point_bonus_per_hit_dice()) {
		return;
	}

	const int old_bonus = this->get_hit_point_bonus_per_hit_dice();

	this->hit_point_bonus_per_hit_dice = bonus;

	if (this->get_hit_dice_count() > 0) {
		int hit_point_change = 0;

		for (const auto &[hit_dice, roll_results] : this->hit_dice_roll_results) {
			for (const int roll_result : roll_results) {
				const int old_hit_point_change = std::max(roll_result + old_bonus, hit_dice.get_count());
				const int new_hit_point_change = std::max(roll_result + bonus, hit_dice.get_count());
				hit_point_change += new_hit_point_change - old_hit_point_change;
			}
		}

		if (hit_point_change != 0) {
			this->change_max_hit_points(hit_point_change, true);
		}
	}
}

void character_game_data::change_hit_point_bonus_per_hit_dice(const int change)
{
	this->set_hit_point_bonus_per_hit_dice(this->get_hit_point_bonus_per_hit_dice() + change);
}

void character_game_data::set_armor_class_bonus(const int bonus)
{
	if (bonus == this->get_armor_class_bonus()) {
		return;
	}

	const int old_bonus = this->get_armor_class_bonus();

	this->armor_class_bonus = bonus;

	//an armor class bonus of +10 increases challenge rating by 1
	if (bonus >= 10 && old_bonus < 10) {
		this->change_challenge_rating(1);
	} else if (bonus < 10 && old_bonus >= 10) {
		this->change_challenge_rating(-1);
	}

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

	static constexpr dice null_dice(0, 0, 0, 0);
	return null_dice;
}

void character_game_data::set_damage_bonus(const int bonus)
{
	if (bonus == this->get_damage_bonus()) {
		return;
	}

	this->damage_bonus = bonus;

	if (game::get()->is_running()) {
		emit damage_bonus_changed();
	}
}

void character_game_data::change_damage_bonus(const int change)
{
	this->set_damage_bonus(this->get_damage_bonus() + change);
}

void character_game_data::set_range(const int range)
{
	if (range == this->get_range()) {
		return;
	}

	this->range = range;

	if (game::get()->is_running()) {
		emit range_changed();
	}
}

void character_game_data::change_range(const int change)
{
	this->set_range(this->get_range() + change);
}

void character_game_data::set_movement(const int movement)
{
	if (movement == this->get_movement()) {
		return;
	}

	this->movement = movement;

	if (game::get()->is_running()) {
		emit movement_changed();
	}
}

void character_game_data::change_movement(const int change)
{
	this->set_movement(this->get_movement() + change);
}

int character_game_data::get_combat_movement() const
{
	int movement = this->get_movement();

	if (movement > 0) {
		movement = std::max(movement / 2, 1);
	}

	return movement;
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

bool character_game_data::do_saving_throw(const saving_throw_type *saving_throw_type, const int roll_modifier) const
{
	static constexpr dice saving_throw_dice(1, 20);

	const int roll_result = random::get()->roll_dice(saving_throw_dice);
	const int modified_roll_result = roll_result + roll_modifier;

	const int saving_throw_value = 20 - this->get_saving_throw_bonus(saving_throw_type);

	return modified_roll_result >= saving_throw_value;
}

bool character_game_data::is_skill_trained(const skill *skill) const
{
	if (!skill->is_trained_only()) {
		return true;
	}

	return this->skill_trainings.contains(skill);
}

void character_game_data::change_skill_training(const skill *skill, const int change)
{
	if (change == 0) {
		return;
	}

	const bool was_trained = this->is_skill_trained(skill);

	const int new_value = (this->skill_trainings[skill] += change);
	assert_throw(new_value >= 0);
	if (new_value == 0) {
		this->skill_trainings.erase(skill);
	}

	const bool is_trained = this->is_skill_trained(skill);

	if (is_trained && !was_trained) {
		this->on_attribute_value_changed(skill, this->get_skill_value(skill), 0);
	} else if (!is_trained && was_trained) {
		this->on_attribute_value_changed(skill, 0, this->get_skill_value(skill));
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

	const int old_value = this->get_skill_value(skill);

	const int new_value = (this->skill_values[skill] += change);
	if (new_value == 0) {
		this->skill_values.erase(skill);
	}

	if (this->is_skill_trained(skill)) {
		this->on_attribute_value_changed(skill, new_value, old_value);
	}

	if (game::get()->is_running()) {
		emit skill_values_changed();
	}
}

int character_game_data::get_effective_skill_value(const skill *skill) const
{
	if (!this->is_skill_trained(skill)) {
		return 0;
	}

	return this->get_skill_value(skill);
}

bool character_game_data::do_skill_check(const skill *skill, const int roll_modifier, const site *location) const
{
	assert_throw(skill != nullptr);
	assert_throw(location != nullptr);

	if (!this->is_skill_trained(skill)) {
		return false;
	}

	const int roll_result = random::get()->roll_dice(skill->get_check_dice());

	//there should always be at least a 5% chance of failure
	if (skill->get_check_dice().get_sides() == 100) {
		if (roll_result >= 96) {
			return false;
		}
	} else if (roll_result == skill->get_check_dice().get_sides()) {
		//e.g. if a 20 is rolled for a d20 roll
		return false;
	}

	const int skill_value = this->get_skill_value(skill);
	const int modified_skill_value = skill_value + roll_modifier + location->get_game_data()->get_skill_modifier(skill);
	return roll_result <= modified_skill_value;
}

int character_game_data::get_skill_check_chance(const skill *skill, const int roll_modifier, const site *location) const
{
	assert_throw(skill != nullptr);
	assert_throw(location != nullptr);

	if (!this->is_skill_trained(skill)) {
		return 0;
	}

	int chance = this->get_skill_value(skill);
	chance += roll_modifier;
	chance += location->get_game_data()->get_skill_modifier(skill);

	if (skill->get_check_dice().get_sides() != 100) {
		chance *= 100;
		chance /= skill->get_check_dice().get_sides();
	}

	chance = std::min(chance, 95);

	return chance;
}

void character_game_data::change_trait_count(const trait *trait, const int change)
{
	if (change == 0) {
		return;
	}

	if (change > 0) {
		if (!this->can_have_trait(trait)) {
			throw std::runtime_error(std::format("Tried to add trait \"{}\" to character \"{}\", for which the trait's conditions are not fulfilled.", trait->get_identifier(), this->character->get_identifier()));
		}
	}

	const int old_value = this->get_trait_count(trait);

	if (change > 0 && old_value == 0) {
		if (!this->can_gain_trait(trait)) {
			throw std::runtime_error(std::format("Tried to add trait \"{}\" to character \"{}\", who cannot gain it.", trait->get_identifier(), this->character->get_identifier()));
		}
	}

	const int new_value = (this->trait_counts[trait] += change);
	if (new_value == 0) {
		this->trait_counts.erase(trait);
	}

	assert_throw(new_value >= 0);

	if (change > 0) {
		if (vector::contains(this->target_traits, trait)) {
			vector::remove_one(this->target_traits, trait);
		}
	}

	if (trait->is_unlimited()) {
		this->on_trait_gained(trait, change);
	} else {
		if (old_value == 0 && new_value > 0) {
			this->on_trait_gained(trait, 1);
		} else if (new_value == 0 && old_value > 0) {
			this->on_trait_gained(trait, -1);
		}
	}

	if (game::get()->is_running()) {
		emit traits_changed();
	}
}

QVariantList character_game_data::get_traits_qvariant_list() const
{
	return container::to_qvariant_list(archimedes::map::get_keys(this->get_trait_counts()));
}

std::vector<const trait *> character_game_data::get_traits_of_type(const trait_type *trait_type) const
{
	std::vector<const trait *> traits;

	for (const auto &[trait, count] : this->get_trait_counts()) {
		if (!vector::contains(trait->get_types(), trait_type)) {
			continue;
		}

		traits.push_back(trait);
	}

	return traits;
}

QVariantList character_game_data::get_traits_of_type(const QString &trait_type_str) const
{
	const trait_type *type = trait_type::get(trait_type_str.toStdString());
	return container::to_qvariant_list(this->get_traits_of_type(type));
}

bool character_game_data::can_have_trait(const trait *trait) const
{
	if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(this->character, read_only_context(this->character))) {
		return false;
	}

	if (trait->get_divine_domain() != nullptr) {
		if (!this->is_deity()) {
			return false;
		}

		if (!vector::contains(this->character->get_deity()->get_major_domains(), trait->get_divine_domain())) {
			return false;
		}
	}

	return true;
}

bool character_game_data::can_gain_trait(const trait *trait) const
{
	if (this->has_trait(trait) && !trait->is_unlimited()) {
		return false;
	}

	if (!this->can_have_trait(trait)) {
		return false;
	}

	if (trait->get_gain_conditions() != nullptr && !trait->get_gain_conditions()->check(this->character, read_only_context(this->character))) {
		return false;
	}

	for (const trait_type *trait_type : trait->get_types()) {
		if (trait_type->get_max_traits() > 0 && this->get_trait_count_for_type(trait_type) >= trait_type->get_max_traits()) {
			return false;
		}


		if (trait_type->get_gain_conditions() != nullptr && !trait_type->get_gain_conditions()->check(this->character, read_only_context(this->character))) {
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

bool character_game_data::has_trait(const trait *trait) const
{
	return this->get_trait_counts().contains(trait);
}

void character_game_data::on_trait_gained(const trait *trait, const int multiplier)
{
	if (this->get_office() != nullptr) {
		assert_throw(this->get_domain() != nullptr);

		if (trait->get_office_modifier(this->get_office()) != nullptr) {
			this->apply_trait_office_modifier(trait, this->get_domain(), this->get_office(), multiplier);
		}
	}

	for (const auto &[attribute, bonus] : trait->get_attribute_bonuses()) {
		this->change_attribute_value(attribute, bonus * multiplier);
	}

	if (trait->get_modifier() != nullptr) {
		this->apply_modifier(trait->get_modifier(), multiplier);
	}

	for (const trait_type *trait_type : trait->get_types()) {
		if (trait_type->get_modifier() != nullptr) {
			this->apply_modifier(trait_type->get_modifier(), multiplier);
		}
	}

	for (const auto &[tier_interval, modifier] : trait->get_per_mythic_tier_modifiers()) {
		const int tier_multiplier = this->character->get_mythic_tier() / tier_interval;
		if (tier_multiplier > 0) {
			this->apply_modifier(modifier.get(), tier_multiplier * multiplier);
		}
	}

	for (const auto &[rank_interval, modifier] : trait->get_per_divine_rank_modifiers()) {
		assert_throw(this->is_deity());

		const int rank_multiplier = this->character->get_deity()->get_divine_level() / rank_interval;
		if (rank_multiplier > 0) {
			this->apply_modifier(modifier.get(), rank_multiplier * multiplier);
		}
	}

	if (trait->get_military_unit_modifier() != nullptr && this->get_military_unit() != nullptr) {
		this->apply_military_unit_modifier(this->get_military_unit(), multiplier);
	}

	if (trait->get_divine_domain() != nullptr) {
		assert_throw(this->is_deity());
		assert_throw(vector::contains(this->character->get_deity()->get_major_domains(), trait->get_divine_domain()));
	}
}

void character_game_data::add_trait_of_type(const trait_type *trait_type)
{
	try {
		std::vector<const trait *> potential_traits = this->get_potential_traits_from_list(vector::intersected(this->target_traits, trait_type->get_traits()));

		if (potential_traits.empty()) {
			potential_traits = this->get_potential_traits_from_list(trait_type->get_traits());

			if (this->character == game::get()->get_player_character()) {
				potential_traits.push_back(nullptr);
			} else {
				for (int i = 0; i < trait_type->get_none_weight(); ++i) {
					potential_traits.push_back(nullptr);
				}
			}
		}

		//there should always be enough choosable traits
		assert_throw(!potential_traits.empty());

		if (this->character == game::get()->get_player_character()) {
			std::sort(potential_traits.begin(), potential_traits.end(), [](const trait *lhs, const trait *rhs) {
				if (lhs != nullptr && rhs != nullptr) {
					return lhs->get_identifier() < rhs->get_identifier();
				} else if ((lhs == nullptr) != (rhs == nullptr)) {
					return lhs != nullptr;
				}

				return false;
			});

			emit engine_interface::get()->trait_choosable(this->character, trait_type, container::to_qvariant_list(potential_traits));
		} else {
			const trait *chosen_trait = vector::get_random(potential_traits);
			this->on_trait_chosen(chosen_trait, trait_type);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to add trait of type \"{}\" for character \"{}\".", trait_type->get_identifier(), this->character->get_identifier())));
	}
}

void character_game_data::remove_trait_of_type(const trait_type *trait_type)
{
	try {
		std::vector<const trait *> &chosen_traits = this->trait_choices[trait_type];
		assert_throw(!chosen_traits.empty());

		if (chosen_traits.back() != nullptr) {
			this->change_trait_count(chosen_traits.back(), -1);
		}
		chosen_traits.pop_back();

		if (chosen_traits.empty()) {
			this->trait_choices.erase(trait_type);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to remove trait of type \"{}\" for character \"{}\".", trait_type->get_identifier(), this->character->get_identifier())));
	}
}

bool character_game_data::generate_trait(const trait_type *trait_type, const character_attribute *target_attribute, const int target_attribute_bonus)
{
	std::vector<const trait *> potential_traits;
	int best_attribute_bonus = 0;

	for (const trait *trait : trait::get_all()) {
		if (!vector::contains(trait->get_types(), trait_type)) {
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

	this->change_trait_count(vector::get_random(potential_traits), 1);
	return true;
}

bool character_game_data::generate_initial_trait(const trait_type *trait_type)
{
	const character_attribute *target_attribute = this->character->get_skill() != 0 ? this->character->get_primary_attribute() : nullptr;
	const int target_attribute_value = target_attribute ? this->character->get_skill() : 0;
	const int target_attribute_bonus = target_attribute ? (target_attribute_value - this->get_attribute_value(target_attribute)) : 0;

	return this->generate_trait(trait_type, target_attribute, target_attribute_bonus);
}

void character_game_data::on_trait_chosen(const trait *trait, const trait_type *trait_type)
{
	this->trait_choices[trait_type].push_back(trait);

	if (trait != nullptr) {
		this->change_trait_count(trait, 1);
	}
}

std::vector<const trait *> character_game_data::get_potential_traits_from_list(const std::vector<const trait *> &traits) const
{
	std::vector<const trait *> potential_traits;
	bool found_unacquired_trait = false;

	for (const trait *trait : traits) {
		if (trait == nullptr) {
			potential_traits.push_back(trait);
			continue;
		}

		if (!this->can_gain_trait(trait)) {
			continue;
		}

		if (this->character == game::get()->get_player_character()) {
			potential_traits.push_back(trait);
		} else {
			if (!found_unacquired_trait && !this->has_trait(trait)) {
				potential_traits.clear();
				found_unacquired_trait = true;
			} else if (found_unacquired_trait && this->has_trait(trait)) {
				continue;
			}

			const int weight = trait->get_weight_factor() != nullptr ? trait->get_weight_factor()->calculate(this->character).to_int() : 1;

			for (int i = 0; i < weight; ++i) {
				potential_traits.push_back(trait);
			}
		}
	}

	return potential_traits;
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

	if (office != nullptr && office->is_ruler()) {
		this->get_domain()->get_economy()->change_wealth(this->wealth);
		this->wealth = 0;
	}

	if (game::get()->is_running()) {
		emit office_changed();
	}
}

std::string character_game_data::get_office_modifier_string(const metternich::domain *domain, const metternich::office *office) const
{
	assert_throw(office != nullptr);

	std::string str;

	for (const auto &[trait, count] : this->get_trait_counts()) {
		if (trait->get_office_modifier(office) == nullptr) {
			continue;
		}

		const size_t indent = 1;

		std::string trait_modifier_str;

		if (trait->get_office_modifier(office) != nullptr) {
			trait_modifier_str = trait->get_office_modifier(office)->get_string(domain, trait->is_unlimited() ? count : 1, indent);
		}

		if (trait_modifier_str.empty()) {
			continue;
		}

		if (!str.empty()) {
			str += "\n";
		}

		if (!str.empty()) {
			str += "\n";
		}

		str += string::highlight(trait->get_name());

		str += "\n" + trait_modifier_str;
	}

	return str;
}

void character_game_data::apply_office_modifier(const metternich::domain *domain, const metternich::office *office, const int multiplier) const
{
	assert_throw(domain != nullptr);
	assert_throw(office != nullptr);

	for (const auto &[trait, count] : this->get_trait_counts()) {
		this->apply_trait_office_modifier(trait, domain, office, (trait->is_unlimited() ? count : 1) * multiplier);
	}
}

void character_game_data::apply_trait_office_modifier(const trait *trait, const metternich::domain *domain, const metternich::office *office, const int multiplier) const
{
	if (trait->get_office_modifier(office) != nullptr) {
		trait->get_office_modifier(office)->apply(domain, multiplier);
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
	assert_throw(this->character->get_military_unit_category() != military_unit_category::none);

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

	if (this->get_military_unit() != nullptr) {
		this->get_military_unit()->disband(false);
	} else if (this->get_civilian_unit() != nullptr) {
		this->get_civilian_unit()->disband(false);
	}
}

void character_game_data::apply_modifier(const modifier<const metternich::character> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	modifier->apply(this->character, multiplier);
}

void character_game_data::apply_military_unit_modifier(metternich::military_unit *military_unit, const int multiplier)
{
	for (const auto &[trait, count] : this->get_trait_counts()) {
		if (trait->get_military_unit_modifier() != nullptr) {
			trait->get_military_unit_modifier()->apply(military_unit, (trait->is_unlimited() ? count : 1) * multiplier);
		}
	}
}

QVariantList character_game_data::get_spells_qvariant_list() const
{
	return container::to_qvariant_list(this->get_spells());
}

bool character_game_data::has_spell(const spell *spell) const
{
	return vector::contains(this->get_spells(), spell);
}

bool character_game_data::can_learn_spell(const spell *spell) const
{
	if (!spell->is_available_for_character_class(this->get_character_class())) {
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

void character_game_data::sort_spells()
{
	std::sort(this->spells.begin(), this->spells.end(), [](const spell *lhs, const spell *rhs) {
		if (lhs->get_level() != rhs->get_level()) {
			return lhs->get_level() < rhs->get_level();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

int character_game_data::get_wealth() const
{
	if (this->is_ruler()) {
		assert_throw(this->wealth == 0);
		return this->get_domain()->get_economy()->get_wealth();
	} else {
		return this->wealth;
	}
}

void character_game_data::change_wealth(const int change)
{
	if (this->is_ruler()) {
		assert_throw(this->wealth == 0);
		this->get_domain()->get_economy()->change_wealth(change);
	} else {
		this->wealth += change;
		emit wealth_changed();
	}
}

QVariantList character_game_data::get_battle_spells_qvariant_list() const
{
	std::vector<const spell *> spells = container::to_vector(this->get_spells());

	std::erase_if(spells, [](const spell *spell) {
		return !spell->is_battle_spell();
	});

	return container::to_qvariant_list(spells);
}

QVariantList character_game_data::get_items_qvariant_list() const
{
	return container::to_qvariant_list(this->get_items());
}

bool character_game_data::has_item(const item_type *item_type) const
{
	for (const qunique_ptr<item> &item : this->get_items()) {
		if (item->get_type() == item_type) {
			return true;
		}
	}

	return false;
}

void character_game_data::add_item(qunique_ptr<item> &&item)
{
	if (item->get_type()->is_stackable()) {
		for (const qunique_ptr<metternich::item> &loop_item : this->get_items()) {
			if (loop_item->get_type() == item->get_type() && loop_item->get_material() == item->get_material() && loop_item->get_enchantment() == item->get_enchantment() && loop_item->get_spell() == item->get_spell()) {
				loop_item->change_quantity(1);
				emit items_changed();
				return;
			}
		}
	}

	metternich::item *item_ptr = item.get();
	this->items.push_back(std::move(item));
	emit items_changed();

	if (this->can_equip_item(item_ptr, false)) {
		this->equip_item(item_ptr);
	} else if (this->is_ai() && this->can_use_item(item_ptr)) {
		//AI characters use items as soon as they receive them
		this->use_item(item_ptr);
	}
}

qunique_ptr<item> character_game_data::take_item(metternich::item *item)
{
	qunique_ptr<metternich::item> taken_item;

	if (item->get_type()->is_stackable() && item->get_quantity() > 1) {
		item->change_quantity(-1);
		taken_item = make_qunique<metternich::item>(item->get_type(), item->get_material(), item->get_enchantment(), item->get_spell());
		emit items_changed();
		return taken_item;
	}

	if (item->is_equipped()) {
		this->deequip_item(item);
	}

	for (auto it = this->items.begin(); it != this->items.end(); ++it) {
		if (it->get() == item) {
			taken_item = std::move(*it);
			this->items.erase(it);
			break;
		}
	}

	emit items_changed();
	return taken_item;
}

void character_game_data::remove_item(item *item)
{
	if (item->get_type()->is_stackable()) {
		item->change_quantity(-1);
		if (item->get_quantity() > 0) {
			emit items_changed();
			return;
		}
	}

	if (item->is_equipped()) {
		this->deequip_item(item);
	}

	vector::remove(this->items, item);
	emit items_changed();
}

void character_game_data::remove_item(const item_type *item_type, const item_material *material, const enchantment *enchantment, const spell *spell)
{
	for (const qunique_ptr<item> &item : this->get_items()) {
		if (item->get_type() == item_type && item->get_material() == material && item->get_enchantment() == enchantment && item->get_spell() == spell) {
			this->remove_item(item.get());
			return;
		}
	}
}

bool character_game_data::can_use_item(const metternich::item *item) const
{
	if (!item->get_type()->get_item_class()->is_consumable()) {
		return false;
	}

	if (item->get_enchantment() != nullptr && item->get_enchantment()->get_conditions() != nullptr) {
		if (!item->get_enchantment()->get_conditions()->check(this->character, read_only_context(this->character))) {
			return false;
		}
	}

	return true;
}

void character_game_data::use_item(metternich::item *item)
{
	assert_throw(this->can_use_item(item));

	this->on_item_used(item);

	this->remove_item(item);
}

void character_game_data::on_item_used(const item *item)
{
	const item_type *type = item->get_type();
	if (type->get_modifier() != nullptr) {
		type->get_modifier()->apply(this->character);
	}

	if (item->get_enchantment() != nullptr) {
		this->on_item_used_with_enchantment(item->get_enchantment());
	}
}

void character_game_data::on_item_used_with_enchantment(const enchantment *enchantment)
{
	if (enchantment->get_modifier() != nullptr) {
		enchantment->get_modifier()->apply(this->character);
	}

	for (const metternich::enchantment *subenchantment : enchantment->get_subenchantments()) {
		this->on_item_used_with_enchantment(subenchantment);
	}
}

bool character_game_data::can_equip_item(const item *item, const bool ignore_already_equipped) const
{
	const item_slot *slot = item->get_slot();
	if (slot == nullptr) {
		return false;
	}

	if (item->get_enchantment() != nullptr && item->get_enchantment()->get_conditions() != nullptr) {
		if (!item->get_enchantment()->get_conditions()->check(this->character, read_only_context(this->character))) {
			return false;
		}
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

QVariantList character_game_data::get_status_effects_qvariant_list() const
{
	return container::to_qvariant_list(archimedes::map::get_keys(this->status_effect_rounds));
}

void character_game_data::set_status_effect_rounds(const status_effect *status_effect, const int rounds)
{
	if (rounds == this->get_status_effect_rounds(status_effect)) {
		return;
	}

	if (rounds == 0) {
		this->status_effect_rounds.erase(status_effect);
	} else {
		this->status_effect_rounds[status_effect] = rounds;
	}

	if (game::get()->is_running()) {
		emit status_effect_rounds_changed();
	}
}

void character_game_data::decrement_status_effect_rounds()
{
	const std::vector<const status_effect *> status_effects = archimedes::map::get_keys(this->status_effect_rounds);

	for (const status_effect *status_effect : status_effects) {
		this->change_status_effect_rounds(status_effect, -1);

		if (this->get_status_effect_rounds(status_effect) == 0 && status_effect->get_end_effects() != nullptr) {
			context ctx(this->character);
			ctx.in_combat = true; //must be in combat for status effect rounds to be decremented
			status_effect->get_end_effects()->do_effects(this->character, ctx);
		}
	}
}

const site *character_game_data::get_location() const
{
	if (this->get_civilian_unit() != nullptr && this->get_civilian_unit()->get_province()->get_game_data()->get_provincial_capital() != nullptr) {
		return this->get_civilian_unit()->get_province()->get_game_data()->get_provincial_capital();
	}

	if (this->get_military_unit() != nullptr) {
		if (this->get_military_unit()->get_province()->get_game_data()->get_provincial_capital() != nullptr) {
			return this->get_military_unit()->get_province()->get_game_data()->get_provincial_capital();
		}
	}

	if (this->get_domain() != nullptr) {
		return this->get_domain()->get_game_data()->get_capital();
	}

	return nullptr;
}

void character_game_data::ai_buy_items()
{
	if (this->get_wealth() <= 0) {
		return;
	}

	const province *province = this->get_location()->get_game_data()->get_province();
	assert_throw(province != nullptr);

	for (const site *holding_site : province->get_game_data()->get_settlement_sites()) {
		if (!holding_site->get_game_data()->is_built()) {
			continue;
		}

		for (building_item_slot *item_slot : holding_site->get_game_data()->get_item_slots()) {
			if (item_slot->get_item() == nullptr) {
				continue;
			}

			if (!item_slot->can_buy_item(this->character)) {
				continue;
			}

			if (!item_slot->get_item()->is_useful_for(this->character)) {
				continue;
			}

			item_slot->buy_item(this->character);
		}
	}
}

}
