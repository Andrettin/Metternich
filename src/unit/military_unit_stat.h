#pragma once

namespace metternich {

enum class military_unit_stat {
	hit_points,
	movement,
	defense,
	melee,
	charge,
	missile,
	range,
	save,
	melee_vs_mounted,
	defense_vs_mounted,
	cargo, //the cargo capacity of the military unit/transporter

	count
};

inline std::string_view get_military_unit_stat_name(const military_unit_stat stat)
{
	switch (stat) {
		case military_unit_stat::hit_points:
			return "Hit Points";
		case military_unit_stat::movement:
			return "Movement";
		case military_unit_stat::defense:
			return "Defense";
		case military_unit_stat::melee:
			return "Melee";
		case military_unit_stat::charge:
			return "Charge";
		case military_unit_stat::missile:
			return "Missile";
		case military_unit_stat::range:
			return "Range";
		case military_unit_stat::save:
			return "Save";
		case military_unit_stat::melee_vs_mounted:
			return "Melee vs. Mounted";
		case military_unit_stat::defense_vs_mounted:
			return "Defense vs. Mounted";
		case military_unit_stat::cargo:
			return "Cargo";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit stat: \"{}\".", std::to_underlying(stat)));
}

inline std::string_view get_military_unit_stat_short_name(const military_unit_stat stat)
{
	switch (stat) {
		case military_unit_stat::hit_points:
			return "HP";
		case military_unit_stat::movement:
			return "Move";
		case military_unit_stat::defense:
			return "Defense";
		case military_unit_stat::melee:
			return "Melee";
		case military_unit_stat::charge:
			return "Charge";
		case military_unit_stat::missile:
			return "Missile";
		case military_unit_stat::range:
			return "Range";
		case military_unit_stat::save:
			return "Save";
		case military_unit_stat::melee_vs_mounted:
			return "Melee vs. Mounted";
		case military_unit_stat::defense_vs_mounted:
			return "Defense vs. Mounted";
		case military_unit_stat::cargo:
			return "Cargo";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit stat: \"{}\".", std::to_underlying(stat)));
}

inline bool is_percent_military_unit_stat(const military_unit_stat stat)
{
	Q_UNUSED(stat);

	return false;
}

}

Q_DECLARE_METATYPE(metternich::military_unit_stat)
