#pragma once

namespace metternich {

enum class character_role {
	advisor,
	leader,
	civilian
};

inline std::string_view get_character_role_name(const character_role role)
{
	switch (role) {
		case character_role::advisor:
			return "Advisor";
		case character_role::leader:
			return "Leader";
		case character_role::civilian:
			return "Civilian";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid character role: \"{}\".", static_cast<int>(role)));
}

}

Q_DECLARE_METATYPE(metternich::character_role)
