#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_domain {
	none,
	land,
	water,
	air
};

inline std::string get_military_unit_domain_name(const military_unit_domain domain)
{
	switch (domain) {
		case military_unit_domain::land:
			return "Land";
		case military_unit_domain::water:
			return "Water";
		case military_unit_domain::air:
			return "Air";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit domain: \"{}\".", std::to_string(static_cast<int>(domain))));
}

}

extern template class archimedes::enum_converter<metternich::military_unit_domain>;

Q_DECLARE_METATYPE(metternich::military_unit_domain)
