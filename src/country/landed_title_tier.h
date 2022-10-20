#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class landed_title_tier {
	none,
	barony,
	viscounty,
	county,
	marquisate,
	duchy,
	grand_duchy,
	kingdom,
	empire
};

inline std::string get_landed_title_tier_name(const landed_title_tier tier)
{
	switch (tier) {
		case landed_title_tier::none:
			return "None";
		case landed_title_tier::barony:
			return "Barony";
		case landed_title_tier::viscounty:
			return "Viscounty";
		case landed_title_tier::county:
			return "County";
		case landed_title_tier::marquisate:
			return "Marquisate";
		case landed_title_tier::duchy:
			return "Duchy";
		case landed_title_tier::grand_duchy:
			return "Grand Duchy";
		case landed_title_tier::kingdom:
			return "Kingdom";
		case landed_title_tier::empire:
			return "Empire";
		default:
			break;
	}

	throw std::runtime_error("Invalid faction tier: \"" + std::to_string(static_cast<int>(tier)) + "\".");
}

}

extern template class archimedes::enum_converter<metternich::landed_title_tier>;

Q_DECLARE_METATYPE(metternich::landed_title_tier)
