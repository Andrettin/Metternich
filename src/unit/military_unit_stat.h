#pragma once

namespace metternich {

enum class military_unit_stat {
	hit_points,
	movement,
	defense,
	melee,
	charge,
	missile,

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
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit stat: \"{}\".", std::to_string(static_cast<int>(stat))));
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
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit stat: \"{}\".", std::to_string(static_cast<int>(stat))));
}

inline bool is_percent_military_unit_stat(const military_unit_stat stat)
{
	Q_UNUSED(stat);

	return false;
}

}

Q_DECLARE_METATYPE(metternich::military_unit_stat)
