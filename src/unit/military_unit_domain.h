#pragma once

namespace metternich {

enum class military_unit_domain {
	none,
	land,
	water,
	air,
	space
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
		case military_unit_domain::space:
			return "Space";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit domain: \"{}\".", std::to_string(static_cast<int>(domain))));
}

}

Q_DECLARE_METATYPE(metternich::military_unit_domain)
