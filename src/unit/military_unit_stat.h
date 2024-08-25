#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_stat {
	firepower,
	melee,
	shock, //morale damage bonus
	range,
	defense,
	resistance, //resistance to damage
	discipline, //resistance to morale damage
	movement,
	damage_bonus,
	bonus_vs_infantry,
	bonus_vs_cavalry,
	bonus_vs_artillery,
	bonus_vs_fortifications,
	ranged_defense_modifier,
	entrenchment_bonus_modifier,
	desert_attack_modifier,
	desert_defense_modifier,
	forest_attack_modifier,
	forest_defense_modifier,
	hills_attack_modifier,
	hills_defense_modifier,
	mountains_attack_modifier,
	mountains_defense_modifier,
	wetland_attack_modifier,
	wetland_defense_modifier,

	count
};

inline std::string_view get_military_unit_stat_name(const military_unit_stat stat)
{
	switch (stat) {
		case military_unit_stat::firepower:
			return "Firepower";
		case military_unit_stat::melee:
			return "Melee";
		case military_unit_stat::shock:
			return "Shock";
		case military_unit_stat::range:
			return "Range";
		case military_unit_stat::defense:
			return "Defense";
		case military_unit_stat::resistance:
			return "Resistance";
		case military_unit_stat::discipline:
			return "Discipline";
		case military_unit_stat::movement:
			return "Movement";
		case military_unit_stat::damage_bonus:
			return "Damage Bonus";
		case military_unit_stat::bonus_vs_infantry:
			return "Bonus vs. Infantry";
		case military_unit_stat::bonus_vs_cavalry:
			return "Bonus vs. Cavalry";
		case military_unit_stat::bonus_vs_artillery:
			return "Bonus vs. Artillery";
		case military_unit_stat::bonus_vs_fortifications:
			return "Bonus vs. Fortifications";
		case military_unit_stat::ranged_defense_modifier:
			return "Ranged Defense";
		case military_unit_stat::entrenchment_bonus_modifier:
			return "Entrenchment Bonus Modifier";
		case military_unit_stat::desert_attack_modifier:
			return "Desert Attack Modifier";
		case military_unit_stat::desert_defense_modifier:
			return "Desert Defense Modifier";
		case military_unit_stat::forest_attack_modifier:
			return "Forest Attack Modifier";
		case military_unit_stat::forest_defense_modifier:
			return "Forest Defense Modifier";
		case military_unit_stat::hills_attack_modifier:
			return "Hills Attack Modifier";
		case military_unit_stat::hills_defense_modifier:
			return "Hills Defense Modifier";
		case military_unit_stat::mountains_attack_modifier:
			return "Mountains Attack Modifier";
		case military_unit_stat::mountains_defense_modifier:
			return "Mountains Defense Modifier";
		case military_unit_stat::wetland_attack_modifier:
			return "Wetland Attack Modifier";
		case military_unit_stat::wetland_defense_modifier:
			return "Wetland Defense Modifier";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit stat: \"{}\".", std::to_string(static_cast<int>(stat))));
}

inline bool is_percent_military_unit_stat(const military_unit_stat stat)
{
	switch (stat) {
		case military_unit_stat::shock:
		case military_unit_stat::resistance:
		case military_unit_stat::discipline:
		case military_unit_stat::damage_bonus:
		case military_unit_stat::bonus_vs_infantry:
		case military_unit_stat::bonus_vs_cavalry:
		case military_unit_stat::bonus_vs_artillery:
		case military_unit_stat::bonus_vs_fortifications:
		case military_unit_stat::ranged_defense_modifier:
		case military_unit_stat::entrenchment_bonus_modifier:
		case military_unit_stat::desert_attack_modifier:
		case military_unit_stat::desert_defense_modifier:
		case military_unit_stat::forest_attack_modifier:
		case military_unit_stat::forest_defense_modifier:
		case military_unit_stat::hills_attack_modifier:
		case military_unit_stat::hills_defense_modifier:
		case military_unit_stat::mountains_attack_modifier:
		case military_unit_stat::mountains_defense_modifier:
		case military_unit_stat::wetland_attack_modifier:
		case military_unit_stat::wetland_defense_modifier:
			return true;
		default:
			return false;
	}
}

}

extern template class archimedes::enum_converter<metternich::military_unit_stat>;

Q_DECLARE_METATYPE(metternich::military_unit_stat)
