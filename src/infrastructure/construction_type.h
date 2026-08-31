#pragma once

namespace metternich {

enum class construction_type {
	none,
	fortification,
};

inline std::string_view get_construction_type_name(const construction_type construction_type)
{
	switch (construction_type) {
		case construction_type::fortification:
			return "Fortification";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid construction type: \"{}\".", std::to_underlying(construction_type)));
}

}

Q_DECLARE_METATYPE(metternich::construction_type)
