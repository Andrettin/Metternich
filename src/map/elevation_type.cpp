#include "metternich.h"

#include "map/elevation_type.h"

namespace archimedes {

template class enum_converter<metternich::elevation_type>;

template <>
const std::string enum_converter<metternich::elevation_type>::property_class_identifier = "metternich::elevation_type";

template <>
const std::map<std::string, metternich::elevation_type> enum_converter<metternich::elevation_type>::string_to_enum_map = {
	{ "none", metternich::elevation_type::none },
	{ "water", metternich::elevation_type::water },
	{ "flatlands", metternich::elevation_type::flatlands },
	{ "hills", metternich::elevation_type::hills },
	{ "mountains", metternich::elevation_type::mountains }
};

template <>
const bool enum_converter<metternich::elevation_type>::initialized = enum_converter::initialize();

}
