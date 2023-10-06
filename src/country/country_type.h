#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class country_type {
	great_power,
	minor_nation,
	tribe
};

inline std::string get_country_type_name(const country_type type)
{
	switch (type) {
		case country_type::great_power:
			return "Great Power";
		case country_type::minor_nation:
		case country_type::tribe:
			return "Minor Nation";
		default:
			break;
	}

	throw std::runtime_error("Invalid country type: \"" + std::to_string(static_cast<int>(type)) + "\".");
}

}

extern template class archimedes::enum_converter<metternich::country_type>;

Q_DECLARE_METATYPE(metternich::country_type)
