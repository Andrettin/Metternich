#pragma once

namespace metternich {

enum class country_type {
	clade,
	tribe,
	polity
};

inline std::string get_country_type_name(const country_type type)
{
	switch (type) {
		case country_type::clade:
			return "Clade";
		case country_type::tribe:
			return "Tribe";
		case country_type::polity:
			return "Polity";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid country type: \"{}\".", static_cast<int>(type)));
}

}

Q_DECLARE_METATYPE(metternich::country_type)
