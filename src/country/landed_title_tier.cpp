#include "metternich.h"

#include "country/landed_title_tier.h"

namespace archimedes {

template class enum_converter<metternich::landed_title_tier>;

template <>
const std::string enum_converter<metternich::landed_title_tier>::property_class_identifier = "metternich::landed_title_tier";

template <>
const std::map<std::string, metternich::landed_title_tier> enum_converter<metternich::landed_title_tier>::string_to_enum_map = {
	{ "none", metternich::landed_title_tier::none },
	{ "barony", metternich::landed_title_tier::barony },
	{ "viscounty", metternich::landed_title_tier::viscounty },
	{ "county", metternich::landed_title_tier::county },
	{ "marquisate", metternich::landed_title_tier::marquisate },
	{ "duchy", metternich::landed_title_tier::duchy },
	{ "grand_duchy", metternich::landed_title_tier::grand_duchy },
	{ "kingdom", metternich::landed_title_tier::kingdom },
	{ "empire", metternich::landed_title_tier::empire }
};

template <>
const bool enum_converter<metternich::landed_title_tier>::initialized = enum_converter::initialize();

}
