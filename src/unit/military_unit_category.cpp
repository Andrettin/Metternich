#include "metternich.h"

#include "unit/military_unit_category.h"

namespace archimedes {

template class enum_converter<metternich::military_unit_category>;

template <>
const std::string enum_converter<metternich::military_unit_category>::property_class_identifier = "metternich::military_unit_category";

template <>
const std::map<std::string, metternich::military_unit_category> enum_converter<metternich::military_unit_category>::string_to_enum_map = {
	{ "beasts", metternich::military_unit_category::beasts },
	{ "colossal_beasts", metternich::military_unit_category::colossal_beasts },
	{ "sea_beasts", metternich::military_unit_category::sea_beasts },
	{ "colossal_sea_beasts", metternich::military_unit_category::colossal_sea_beasts },
	{ "flying_beasts", metternich::military_unit_category::flying_beasts },
	{ "colossal_flying_beasts", metternich::military_unit_category::colossal_flying_beasts },
	{ "militia", metternich::military_unit_category::militia },
	{ "mace_infantry", metternich::military_unit_category::mace_infantry },
	{ "spear_infantry", metternich::military_unit_category::spear_infantry },
	{ "blade_infantry", metternich::military_unit_category::blade_infantry },
	{ "light_infantry", metternich::military_unit_category::light_infantry },
	{ "regular_infantry", metternich::military_unit_category::regular_infantry },
	{ "heavy_infantry", metternich::military_unit_category::heavy_infantry },
	{ "bowmen", metternich::military_unit_category::bowmen },
	{ "light_cavalry", metternich::military_unit_category::light_cavalry },
	{ "heavy_cavalry", metternich::military_unit_category::heavy_cavalry },
	{ "spear_cavalry", metternich::military_unit_category::spear_cavalry },
	{ "light_artillery", metternich::military_unit_category::light_artillery },
	{ "heavy_artillery", metternich::military_unit_category::heavy_artillery },
	{ "combat_engineers", metternich::military_unit_category::combat_engineers },
	{ "general", metternich::military_unit_category::general },
	{ "alchemist", metternich::military_unit_category::alchemist },
	{ "bard", metternich::military_unit_category::bard },
	{ "cleric", metternich::military_unit_category::cleric },
	{ "mage", metternich::military_unit_category::mage },
	{ "paladin", metternich::military_unit_category::paladin },
	{ "ranger", metternich::military_unit_category::ranger },
	{ "rogue", metternich::military_unit_category::rogue },
	{ "warrior", metternich::military_unit_category::warrior },
	{ "light_warship", metternich::military_unit_category::light_warship },
	{ "heavy_warship", metternich::military_unit_category::heavy_warship }
};

template <>
const bool enum_converter<metternich::military_unit_category>::initialized = enum_converter::initialize();

}
