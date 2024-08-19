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
	morale_resistance, //resistance to morale damage
	movement,
	damage_bonus,
	bonus_vs_infantry,
	bonus_vs_cavalry,
	bonus_vs_artillery,
	bonus_vs_fortifications,
	entrench_bonus_modifier,

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
		case military_unit_stat::morale_resistance:
			return "Morale Resistance";
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
		case military_unit_stat::entrench_bonus_modifier:
			return "Entrenchment Bonus Modifier";
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
		case military_unit_stat::morale_resistance:
		case military_unit_stat::damage_bonus:
		case military_unit_stat::bonus_vs_infantry:
		case military_unit_stat::bonus_vs_cavalry:
		case military_unit_stat::bonus_vs_artillery:
		case military_unit_stat::bonus_vs_fortifications:
		case military_unit_stat::entrench_bonus_modifier:
			return true;
		default:
			return false;
	}
}

}

extern template class archimedes::enum_converter<metternich::military_unit_stat>;

Q_DECLARE_METATYPE(metternich::military_unit_stat)
