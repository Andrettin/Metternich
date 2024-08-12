#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_category {
	none,
	beasts,
	colossal_beasts,
	sea_beasts,
	colossal_sea_beasts,
	flying_beasts,
	colossal_flying_beasts,
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
	alchemist,
	bard,
	cleric,
	mage,
	paladin,
	ranger,
	rogue,
	warrior,
	light_warship,
	heavy_warship
};

inline std::string_view get_military_unit_category_name(const military_unit_category category)
{
	switch (category) {
		case military_unit_category::beasts:
			return "Beasts";
		case military_unit_category::colossal_beasts:
			return "Colossal Beasts";
		case military_unit_category::sea_beasts:
			return "Sea Beasts";
		case military_unit_category::colossal_sea_beasts:
			return "Colossal Sea Beasts";
		case military_unit_category::flying_beasts:
			return "Flying Beasts";
		case military_unit_category::colossal_flying_beasts:
			return "Colossal Flying Beasts";
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
		case military_unit_category::alchemist:
			return "Alchemist";
		case military_unit_category::bard:
			return "Bard";
		case military_unit_category::cleric:
			return "Cleric";
		case military_unit_category::mage:
			return "Mage";
		case military_unit_category::ranger:
			return "Ranger";
		case military_unit_category::rogue:
			return "Rogue";
		case military_unit_category::paladin:
			return "Paladin";
		case military_unit_category::warrior:
			return "Warrior";
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
		case military_unit_category::alchemist:
		case military_unit_category::bard:
		case military_unit_category::cleric:
		case military_unit_category::mage:
		case military_unit_category::ranger:
		case military_unit_category::rogue:
		case military_unit_category::paladin:
		case military_unit_category::warrior:
			return true;
		default:
			return false;
	}
}

}

extern template class archimedes::enum_converter<metternich::military_unit_category>;

Q_DECLARE_METATYPE(metternich::military_unit_category)
