#pragma once

namespace metternich {

enum class character_attribute_type {
	main,
	personality
};

inline std::string_view get_character_attribute_type_name(const character_attribute_type attribute_type)
{
	switch (attribute_type) {
		case character_attribute_type::main:
			return "Main";
		case character_attribute_type::personality:
			return "Personality";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid character attribute type: {}", static_cast<int>(attribute_type)));
}

}

Q_DECLARE_METATYPE(metternich::character_attribute_type)
