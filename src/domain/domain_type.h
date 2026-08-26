#pragma once

namespace metternich {

enum class domain_type {
	clade,
	tribe,
	polity
};

inline std::string get_domain_type_name(const domain_type type)
{
	switch (type) {
		case domain_type::clade:
			return "Clade";
		case domain_type::tribe:
			return "Tribe";
		case domain_type::polity:
			return "Polity";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid domain type: \"{}\".", std::to_underlying(type)));
}

}

Q_DECLARE_METATYPE(metternich::domain_type)
