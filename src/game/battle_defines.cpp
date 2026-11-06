#include "metternich.h"

#include "game/battle_defines.h"

#include "database/gsml_data.h"
#include "game/attack_result.h"
#include "game/battle_resolution_type.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

battle_defines::battle_defines()
{
}

battle_defines::~battle_defines()
{
}

void battle_defines::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "combat_round_duration") {
		this->combat_round_duration = string::to_duration(value);
	} else if (key == "battle_round_duration") {
		this->battle_round_duration = string::to_duration(value);
	} else if (key == "battle_tile_length") {
		this->battle_tile_length = string::to_length(value);
	} else {
		defines_base::process_gsml_property(property);
	}
}

void battle_defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "military_unit_hit_points_per_hit_dice") {
		scope.for_each_property([this](const gsml_property &property) {
			const int hit_dice_count = std::stoi(property.get_key());
			const centesimal_int hit_points(property.get_value());

			this->military_unit_hit_points_per_hit_dice[hit_dice_count] = hit_points;
		});
	} else if (tag == "battle_resolution_results") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const battle_resolution_type attacker_resolution_type = magic_enum::enum_cast<battle_resolution_type>(child_tag).value();

			child_scope.for_each_child([this, attacker_resolution_type](const gsml_data &grandchild_scope) {
				const std::string &grandchild_tag = grandchild_scope.get_tag();
				const battle_resolution_type defender_resolution_type = magic_enum::enum_cast<battle_resolution_type>(grandchild_tag).value();

				grandchild_scope.for_each_child([this, attacker_resolution_type, defender_resolution_type](const gsml_data &great_grandchild_scope) {
					const int attack_defense_difference = std::stoi(great_grandchild_scope.get_tag());

					std::vector<attack_result> results;
					for (const std::string &value : great_grandchild_scope.get_values()) {
						const attack_result result = magic_enum::enum_cast<attack_result>(value).value();
						results.push_back(result);
					}

					assert_throw(!results.empty());

					this->battle_resolution_results[attacker_resolution_type][defender_resolution_type][attack_defense_difference] = std::move(results);
				});
			});
		});
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

void battle_defines::check() const
{
	if (this->get_combat_round_duration().count() == 0) {
		throw std::runtime_error("No combat round duration has been defined.");
	}

	if (this->get_battle_round_duration().count() == 0) {
		throw std::runtime_error("No battle round duration has been defined.");
	}

	if (this->get_battle_tile_length() == 0) {
		throw std::runtime_error("No battle tile length has been defined.");
	}

	if (this->military_unit_hit_points_per_hit_dice.empty()) {
		throw std::runtime_error("No military unit hit points per hit dice have been defined.");
	}

	if (this->battle_resolution_results.empty()) {
		throw std::runtime_error("No battle resolution results have been defined.");
	}
}

const centesimal_int &battle_defines::get_military_unit_hit_points_for_hit_dice(const int hit_dice_count) const
{
	const auto find_iterator = this->military_unit_hit_points_per_hit_dice.upper_bound(hit_dice_count);
	assert_throw(find_iterator != this->military_unit_hit_points_per_hit_dice.begin());
	return std::prev(find_iterator)->second;
}

const std::vector<attack_result> &battle_defines::get_battle_resolution_results(const battle_resolution_type attacker_resolution_type, const battle_resolution_type defender_resolution_type, const int attack_defense_difference) const
{
	const auto attacker_find_iterator = this->battle_resolution_results.find(attacker_resolution_type);
	assert_throw(attacker_find_iterator != this->battle_resolution_results.end());

	const auto defender_find_iterator = attacker_find_iterator->second.find(defender_resolution_type);
	assert_throw(defender_find_iterator != attacker_find_iterator->second.end());

	assert_throw(!defender_find_iterator->second.empty());

	const auto find_iterator = defender_find_iterator->second.find(attack_defense_difference);
	if (find_iterator != defender_find_iterator->second.end()) {
		return find_iterator->second;
	}

	if (attack_defense_difference < defender_find_iterator->second.begin()->first) {
		return defender_find_iterator->second.begin()->second;
	} else if (attack_defense_difference > defender_find_iterator->second.rbegin()->first) {
		return defender_find_iterator->second.rbegin()->second;
	}

	assert_throw(false);

	static const std::vector<attack_result> empty_vector;
	return empty_vector;
}

}

#include "moc_battle_defines.cpp"
