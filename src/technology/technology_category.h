#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class technology_category {
	none,
	gathering,
	industry,
	army,
	navy,
	finance
};


inline std::string get_technology_category_name(const technology_category category)
{
	switch (category) {
		case technology_category::none:
			return "None";
		case technology_category::gathering:
			return "Gathering";
		case technology_category::industry:
			return "Industry";
		case technology_category::army:
			return "Army";
		case technology_category::navy:
			return "Navy";
		case technology_category::finance:
			return "Finance";
		default:
			break;
	}

	throw std::runtime_error("Invalid technology category: \"" + std::to_string(static_cast<int>(category)) + "\".");
}

}

extern template class archimedes::enum_converter<metternich::technology_category>;

Q_DECLARE_METATYPE(metternich::technology_category)
