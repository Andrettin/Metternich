#include "metternich.h"

#include "map/temperature_type.h"

namespace archimedes {

template class enum_converter<metternich::temperature_type>;

template <>
const std::string enum_converter<metternich::temperature_type>::property_class_identifier = "metternich::temperature_type";

template <>
const std::map<std::string, metternich::temperature_type> enum_converter<metternich::temperature_type>::string_to_enum_map = {
	{ "none", metternich::temperature_type::none },
	{ "frozen", metternich::temperature_type::frozen },
	{ "cold", metternich::temperature_type::cold },
	{ "temperate", metternich::temperature_type::temperate },
	{ "tropical", metternich::temperature_type::tropical }
};

template <>
const bool enum_converter<metternich::temperature_type>::initialized = enum_converter::initialize();

}
