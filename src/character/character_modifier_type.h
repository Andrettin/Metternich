#pragma once

namespace metternich {

enum class character_modifier_type {
	enhancement,
	inherent
};

inline std::string_view get_character_modifier_type_name(const character_modifier_type modifier_type)
{
	switch (modifier_type) {
		case character_modifier_type::enhancement:
			return "Enhancement";
		case character_modifier_type::inherent:
			return "Inherent";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid character modifier type: {}", static_cast<int>(modifier_type)));
}

//FIXME: add support for penalty modifier types, i.e. ones for which the "best" value is the lowest one

}

Q_DECLARE_METATYPE(metternich::character_modifier_type)
