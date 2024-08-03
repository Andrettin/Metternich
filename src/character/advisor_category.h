#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class advisor_category {
	none,
	trade,
	exploration,
	military,
	political,
	religious
};

inline std::string_view get_advisor_category_name(const advisor_category category)
{
	switch (category) {
		case advisor_category::trade:
			return "Trade";
		case advisor_category::exploration:
			return "Exploration";
		case advisor_category::military:
			return "Military";
		case advisor_category::political:
			return "Political";
		case advisor_category::religious:
			return "Religious";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid advisor category: \"{}\".", std::to_string(static_cast<int>(category))));
}

}

extern template class archimedes::enum_converter<metternich::advisor_category>;

Q_DECLARE_METATYPE(metternich::advisor_category)
