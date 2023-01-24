#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class attribute {
	none,
	skill,
	prowess,
	vitality
};

inline std::string get_attribute_name(const attribute attribute)
{
	switch (attribute) {
		case attribute::none:
			return "None";
		case attribute::skill:
			return "Skill";
		case attribute::prowess:
			return "Prowess";
		case attribute::vitality:
			return "Vitality";
		default:
			break;
	}

	throw std::runtime_error("Invalid attribute: \"" + std::to_string(static_cast<int>(attribute)) + "\".");
}

}

extern template class archimedes::enum_converter<metternich::attribute>;

Q_DECLARE_METATYPE(metternich::attribute)
