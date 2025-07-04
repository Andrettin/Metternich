#pragma once

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
	firepower_modifier,
	bonus_vs_infantry,
	bonus_vs_cavalry,
	bonus_vs_artillery,
	bonus_vs_fortifications,
	defense_modifier,
	ranged_defense_modifier,
	entrenchment_bonus_modifier,
	movement_modifier,
	recovery_modifier, //the speed at which strength is recovered
	morale_recovery_modifier, //the speed at which morale is recovered
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
			return "Damage";
		case military_unit_stat::firepower_modifier:
			return "Firepower";
		case military_unit_stat::bonus_vs_infantry:
			return "Bonus vs. Infantry";
		case military_unit_stat::bonus_vs_cavalry:
			return "Bonus vs. Cavalry";
		case military_unit_stat::bonus_vs_artillery:
			return "Bonus vs. Artillery";
		case military_unit_stat::bonus_vs_fortifications:
			return "Bonus vs. Fortifications";
		case military_unit_stat::defense_modifier:
			return "Defense";
		case military_unit_stat::ranged_defense_modifier:
			return "Ranged Defense";
		case military_unit_stat::entrenchment_bonus_modifier:
			return "Entrenchment Bonus";
		case military_unit_stat::movement_modifier:
			return "Movement";
		case military_unit_stat::recovery_modifier:
			return "Recovery";
		case military_unit_stat::morale_recovery_modifier:
			return "Morale Recovery";
		case military_unit_stat::desert_attack_modifier:
			return "Desert Attack";
		case military_unit_stat::desert_defense_modifier:
			return "Desert Defense";
		case military_unit_stat::forest_attack_modifier:
			return "Forest Attack";
		case military_unit_stat::forest_defense_modifier:
			return "Forest Defense";
		case military_unit_stat::hills_attack_modifier:
			return "Hills Attack";
		case military_unit_stat::hills_defense_modifier:
			return "Hills Defense";
		case military_unit_stat::mountains_attack_modifier:
			return "Mountains Attack";
		case military_unit_stat::mountains_defense_modifier:
			return "Mountains Defense";
		case military_unit_stat::wetland_attack_modifier:
			return "Wetland Attack";
		case military_unit_stat::wetland_defense_modifier:
			return "Wetland Defense";
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
		case military_unit_stat::firepower_modifier:
		case military_unit_stat::bonus_vs_infantry:
		case military_unit_stat::bonus_vs_cavalry:
		case military_unit_stat::bonus_vs_artillery:
		case military_unit_stat::bonus_vs_fortifications:
		case military_unit_stat::defense_modifier:
		case military_unit_stat::ranged_defense_modifier:
		case military_unit_stat::entrenchment_bonus_modifier:
		case military_unit_stat::movement_modifier:
		case military_unit_stat::recovery_modifier:
		case military_unit_stat::morale_recovery_modifier:
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

Q_DECLARE_METATYPE(metternich::military_unit_stat)
