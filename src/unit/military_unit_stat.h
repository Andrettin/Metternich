#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_stat {
	firepower,
	melee,
	range,
	defense,
	resistance, //resistance to damage, in percent
	movement,

	count
};

inline std::string_view get_military_unit_stat_name(const military_unit_stat stat)
{
	switch (stat) {
		case military_unit_stat::firepower:
			return "Firepower";
		case military_unit_stat::melee:
			return "Melee";
		case military_unit_stat::range:
			return "Range";
		case military_unit_stat::defense:
			return "Defense";
		case military_unit_stat::resistance:
			return "Resistance";
		case military_unit_stat::movement:
			return "Movement";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit stat: \"{}\".", std::to_string(static_cast<int>(stat))));
}

}

extern template class archimedes::enum_converter<metternich::military_unit_stat>;

Q_DECLARE_METATYPE(metternich::military_unit_stat)
