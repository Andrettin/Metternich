#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_category {
	none,
	beasts,
	colossal_beasts,
	flying_beasts,
	colossal_flying_beasts,
	militia,
	light_infantry,
	regular_infantry,
	heavy_infantry,
	bowmen,
	light_cavalry,
	heavy_cavalry,
	spear_cavalry,
	light_artillery,
	heavy_artillery,
	combat_engineers,
	general,
	light_warship,
	heavy_warship
};

inline std::string get_military_unit_category_name(const military_unit_category category)
{
	switch (category) {
		case military_unit_category::beasts:
			return "Beasts";
		case military_unit_category::colossal_beasts:
			return "Colossal Beasts";
		case military_unit_category::flying_beasts:
			return "Flying Beasts";
		case military_unit_category::colossal_flying_beasts:
			return "Colossal Flying Beasts";
		case military_unit_category::militia:
			return "Militia";
		case military_unit_category::light_infantry:
			return "Light Infantry";
		case military_unit_category::regular_infantry:
			return "Regular Infantry";
		case military_unit_category::heavy_infantry:
			return "Heavy Infantry";
		case military_unit_category::bowmen:
			return "Bowmen";
		case military_unit_category::light_cavalry:
			return "Light Cavalry";
		case military_unit_category::heavy_cavalry:
			return "Heavy Cavalry";
		case military_unit_category::spear_cavalry:
			return "Spear Cavalry";
		case military_unit_category::light_artillery:
			return "Light Artillery";
		case military_unit_category::heavy_artillery:
			return "Heavy Artillery";
		case military_unit_category::combat_engineers:
			return "Combat Engineers";
		case military_unit_category::general:
			return "General";
		case military_unit_category::light_warship:
			return "Light Warship";
		case military_unit_category::heavy_warship:
			return "Heavy Warship";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid military unit category: \"{}\".", std::to_string(static_cast<int>(category))));
}

}

extern template class archimedes::enum_converter<metternich::military_unit_category>;

Q_DECLARE_METATYPE(metternich::military_unit_category)
