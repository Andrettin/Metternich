#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class character_attribute {
	none,
	diplomacy,
	martial,
	stewardship,
	intrigue,
	learning
};

inline std::string_view get_character_attribute_name(const character_attribute attribute)
{
	switch (attribute) {
		case character_attribute::diplomacy:
			return "Diplomacy";
		case character_attribute::martial:
			return "Martial";
		case character_attribute::stewardship:
			return "Stewardship";
		case character_attribute::intrigue:
			return "Intrigue";
		case character_attribute::learning:
			return "Learning";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid character attribute: \"{}\".", static_cast<int>(attribute)));
}

}

extern template class archimedes::enum_converter<metternich::character_attribute>;

Q_DECLARE_METATYPE(metternich::character_attribute)
