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
	{ "flying_beasts", metternich::military_unit_category::flying_beasts },
	{ "colossal_flying_beasts", metternich::military_unit_category::colossal_flying_beasts },
	{ "militia", metternich::military_unit_category::militia },
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
	{ "light_warship", metternich::military_unit_category::light_warship },
	{ "heavy_warship", metternich::military_unit_category::heavy_warship }
};

template <>
const bool enum_converter<metternich::military_unit_category>::initialized = enum_converter::initialize();

}
