#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class country_tier {
	none,
	barony,
	county,
	duchy,
	kingdom,
	empire
};

inline std::string get_country_tier_name(const country_tier tier)
{
	switch (tier) {
		case country_tier::barony:
			return "Barony";
		case country_tier::county:
			return "County";
		case country_tier::duchy:
			return "Duchy";
		case country_tier::kingdom:
			return "Kingdom";
		case country_tier::empire:
			return "Empire";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid country tier: \"{}\".", static_cast<int>(tier)));
}

}

extern template class archimedes::enum_converter<metternich::country_tier>;

Q_DECLARE_METATYPE(metternich::country_tier)
