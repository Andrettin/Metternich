#include "metternich.h"

#include "country/country_tier.h"

namespace archimedes {

template class enum_converter<metternich::country_tier>;

template <>
const std::string enum_converter<metternich::country_tier>::property_class_identifier = "metternich::country_tier";

template <>
const std::map<std::string, metternich::country_tier> enum_converter<metternich::country_tier>::string_to_enum_map = {
	{ "barony", metternich::country_tier::barony },
	{ "county", metternich::country_tier::county },
	{ "duchy", metternich::country_tier::duchy },
	{ "kingdom", metternich::country_tier::kingdom },
	{ "empire", metternich::country_tier::empire }
};

template <>
const bool enum_converter<metternich::country_tier>::initialized = enum_converter::initialize();

}
