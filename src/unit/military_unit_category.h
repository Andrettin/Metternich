#pragma once

namespace metternich {

enum class military_unit_category {
	none,
	militia,
	mace_infantry,
	spear_infantry,
	blade_infantry,
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

inline std::string_view get_military_unit_category_name(const military_unit_category category)
{
	switch (category) {
		case military_unit_category::militia:
			return "Militia";
		case military_unit_category::mace_infantry:
			return "Mace Infantry";
		case military_unit_category::spear_infantry:
			return "Spear Infantry";
		case military_unit_category::blade_infantry:
			return "Blade Infantry";
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

inline bool is_ship_military_unit_category(const military_unit_category category)
{
	switch (category) {
		case military_unit_category::light_warship:
		case military_unit_category::heavy_warship:
			return true;
		default:
			return false;
	}
}

inline bool is_leader_military_unit_category(const military_unit_category category)
{
	switch (category) {
		case military_unit_category::general:
			return true;
		default:
			return false;
	}
}

}

Q_DECLARE_METATYPE(metternich::military_unit_category)
