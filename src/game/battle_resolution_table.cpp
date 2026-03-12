#include "metternich.h"

#include "game/battle_resolution_table.h"

#include "database/gsml_data.h"
#include "game/attack_result.h"
#include "game/battle_resolution_type.h"
#include "util/assert_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

battle_resolution_table::battle_resolution_table(const gsml_data &scope)
{
	scope.for_each_child([this](const gsml_data &child_scope) {
		const std::string &child_tag = child_scope.get_tag();
		const battle_resolution_type attacker_resolution_type = magic_enum::enum_cast<battle_resolution_type>(child_tag).value();

		child_scope.for_each_child([&](const gsml_data &grandchild_scope) {
			const std::string &grandchild_tag = grandchild_scope.get_tag();
			const battle_resolution_type defender_resolution_type = magic_enum::enum_cast<battle_resolution_type>(grandchild_tag).value();

			grandchild_scope.for_each_property([&](const gsml_property &property) {
				const int attack_defense_difference = std::stoi(property.get_key());
				const attack_result result = magic_enum::enum_cast<attack_result>(property.get_value()).value();

				this->results[attacker_resolution_type][defender_resolution_type][attack_defense_difference] = result;
			});
		});
	});
}

attack_result battle_resolution_table::get_result(const battle_resolution_type attacker_resolution_type, const battle_resolution_type defender_resolution_type, const int attack_defense_difference) const
{
	const auto attacker_find_iterator = this->results.find(attacker_resolution_type);
	assert_throw(attacker_find_iterator != this->results.end());

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
	return {};
}

}
