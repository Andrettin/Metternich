#include "metternich.h"

#include "character/character_defines.h"

#include "character/bloodline_strength_category.h"
#include "religion/divine_rank.h"
#include "script/modifier.h"
#include "util/string_conversion_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

character_defines::character_defines()
{
}

character_defines::~character_defines()
{
}

void character_defines::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "battle_movement_rate") {
		this->battle_movement_rate = string::to_length(value);
	} else {
		defines_base::process_gsml_property(property);
	}
}

void character_defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "experience_per_level") {
		scope.for_each_property([this](const gsml_property &property) {
			const int level = std::stoi(property.get_key());
			const int64_t experience = std::stoll(property.get_value());

			this->experience_per_level[level] = experience;
		});
	} else if (tag == "experience_award_per_challenge_rating") {
		scope.for_each_property([this](const gsml_property &property) {
			const int challenge_rating = std::stoi(property.get_key());
			const int64_t experience_award = std::stoll(property.get_value());

			this->experience_award_per_challenge_rating[challenge_rating] = experience_award;
		});
	} else if (tag == "bloodline_strength_category_weights") {
		scope.for_each_property([this](const gsml_property &property) {
			const bloodline_strength_category category = magic_enum::enum_cast<bloodline_strength_category>(property.get_key()).value();
			const int weight = std::stoi(property.get_value());

			this->bloodline_strength_category_weights[category] = weight;

			this->weighted_bloodline_strength_categories.reserve(this->weighted_bloodline_strength_categories.size() + static_cast<size_t>(weight));
			for (int i = 0; i < weight; ++i) {
				this->weighted_bloodline_strength_categories.push_back(category);
			}
		});
	} else if (tag == "bloodline_strength_per_category") {
		scope.for_each_property([this](const gsml_property &property) {
			const bloodline_strength_category category = magic_enum::enum_cast<bloodline_strength_category>(property.get_key()).value();
			dice dice(property.get_value());

			this->bloodline_strength_per_category[category] = std::move(dice);
		});
	} else if (tag == "divine_rank_levels") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->divine_rank_levels[magic_enum::enum_cast<divine_rank>(key).value()] = std::stoi(value);
		});
	} else if (tag == "divine_rank_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const int divine_level = std::stoi(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const character>>();
			modifier->process_gsml_data(child_scope);
			this->divine_rank_modifiers[divine_level] = std::move(modifier);
		});
	} else if (tag == "mana_cost_per_spell_level") {
		scope.for_each_property([this](const gsml_property &property) {
			const int level = std::stoi(property.get_key());
			const int mana_cost = std::stoi(property.get_value());

			this->mana_cost_per_spell_level[level] = mana_cost;
		});
	} else if (tag == "battle_defense_per_armor_class") {
		scope.for_each_property([this](const gsml_property &property) {
			const int armor_class = std::stoi(property.get_key());
			const int defense = std::stoi(property.get_value());

			this->battle_defense_per_armor_class[armor_class] = defense;
		});
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

int character_defines::get_bloodline_strength_category_weight(const bloodline_strength_category category) const
{
	const auto find_iterator = this->bloodline_strength_category_weights.find(category);
	if (find_iterator != this->bloodline_strength_category_weights.end()) {
		return find_iterator->second;
	}

	return 0;
}

const std::vector<bloodline_strength_category> &character_defines::get_weighted_bloodline_strength_categories() const
{
	return this->weighted_bloodline_strength_categories;
}

const dice &character_defines::get_bloodline_strength_for_category(const bloodline_strength_category category) const
{
	const auto find_iterator = this->bloodline_strength_per_category.find(category);
	if (find_iterator != this->bloodline_strength_per_category.end()) {
		return find_iterator->second;
	}

	throw std::runtime_error(std::format("No bloodline strength dice is given for category {}.", magic_enum::enum_name(category)));
}

int character_defines::get_divine_rank_level(const divine_rank rank) const
{
	const auto find_iterator = this->divine_rank_levels.find(rank);

	if (find_iterator != this->divine_rank_levels.end()) {
		return find_iterator->second;
	}

	throw std::runtime_error(std::format("No divine level is given for divine rank \"{}\".", magic_enum::enum_name(rank)));
}

divine_rank character_defines::get_divine_level_rank(const int divine_level) const
{
	divine_rank rank = divine_rank::none;

	for (const auto &[divine_rank, rank_level] : this->divine_rank_levels) {
		if (divine_level >= rank_level) {
			rank = divine_rank;
		} else {
			break;
		}
	}

	return rank;
}

const metternich::modifier<const character> *character_defines::get_divine_rank_modifier(const int divine_rank) const
{
	const auto find_iterator = this->divine_rank_modifiers.find(divine_rank);

	if (find_iterator != this->divine_rank_modifiers.end()) {
		return find_iterator->second.get();
	}

	return nullptr;
}

int character_defines::get_mana_cost_for_spell_level(const int level) const
{
	const auto find_iterator = this->mana_cost_per_spell_level.find(level);

	if (find_iterator != this->mana_cost_per_spell_level.end()) {
		return find_iterator->second;
	}

	throw std::runtime_error(std::format("No mana cost is given for spell level {}.", level));
}

int character_defines::get_battle_defense_for_armor_class(const int armor_class) const
{
	const auto find_iterator = this->battle_defense_per_armor_class.find(armor_class);
	if (find_iterator != this->battle_defense_per_armor_class.end()) {
		return find_iterator->second;
	}

	//use the last value if no specific one was found (if the armor class is greater than the armor class for the last value)
	const auto last_iterator = this->battle_defense_per_armor_class.rbegin();
	if (armor_class > last_iterator->first) {
		return last_iterator->second;
	}

	return 0;
}

}
