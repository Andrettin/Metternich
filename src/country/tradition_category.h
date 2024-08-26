#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class tradition_category {
	none,
	tradition,
	belief
};

inline std::string_view get_tradition_category_name(const tradition_category category)
{
	switch (category) {
		case tradition_category::tradition:
			return "Tradition";
		case tradition_category::belief:
			return "Belief";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid tradition category: \"{}\".", std::to_string(static_cast<int>(category))));
}

}

extern template class archimedes::enum_converter<metternich::tradition_category>;

Q_DECLARE_METATYPE(metternich::tradition_category)
