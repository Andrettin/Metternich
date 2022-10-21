#include "metternich.h"

#include "map/site_type.h"

namespace archimedes {

template class enum_converter<metternich::site_type>;

template <>
const std::string enum_converter<metternich::site_type>::property_class_identifier = "metternich::site_type";

template <>
const std::map<std::string, metternich::site_type> enum_converter<metternich::site_type>::string_to_enum_map = {
	{ "none", metternich::site_type::none },
	{ "settlement", metternich::site_type::settlement },
	{ "terrain", metternich::site_type::terrain },
	{ "resource", metternich::site_type::resource }
};

template <>
const bool enum_converter<metternich::site_type>::initialized = enum_converter::initialize();

}
