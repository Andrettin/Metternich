#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class transporter_stat {
	defense,
	resistance, //resistance to damage
	movement,

	count
};

inline std::string_view get_transporter_stat_name(const transporter_stat stat)
{
	switch (stat) {
		case transporter_stat::defense:
			return "Defense";
		case transporter_stat::resistance:
			return "Resistance";
		case transporter_stat::movement:
			return "Movement";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid transporter stat: \"{}\".", std::to_string(static_cast<int>(stat))));
}

inline bool is_percent_transporter_stat(const transporter_stat stat)
{
	switch (stat) {
		case transporter_stat::resistance:
			return true;
		default:
			return false;
	}
}

}

extern template class archimedes::enum_converter<metternich::transporter_stat>;

Q_DECLARE_METATYPE(metternich::transporter_stat)
