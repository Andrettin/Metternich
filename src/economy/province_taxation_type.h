#pragma once

namespace metternich {

enum class province_taxation_type {
	none,
	no_taxation,
	light_taxation,
	moderate_taxation,
	heavy_taxation
};

inline std::string_view get_province_taxation_type_name(const province_taxation_type province_taxation_type)
{
	switch (province_taxation_type) {
		case province_taxation_type::no_taxation:
			return "No Province Taxation";
		case province_taxation_type::light_taxation:
			return "Light Province Taxation";
		case province_taxation_type::moderate_taxation:
			return "Moderate Province Taxation";
		case province_taxation_type::heavy_taxation:
			return "Heavy Province Taxation";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid province taxation type: {}", std::to_underlying(province_taxation_type)));
}

}

Q_DECLARE_METATYPE(metternich::province_taxation_type)
