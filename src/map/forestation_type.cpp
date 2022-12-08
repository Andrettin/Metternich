#include "metternich.h"

#include "map/forestation_type.h"

namespace archimedes {

template class enum_converter<metternich::forestation_type>;

template <>
const std::string enum_converter<metternich::forestation_type>::property_class_identifier = "metternich::forestation_type";

template <>
const std::map<std::string, metternich::forestation_type> enum_converter<metternich::forestation_type>::string_to_enum_map = {
	{ "none", metternich::forestation_type::none },
	{ "forest", metternich::forestation_type::forest }
};

template <>
const bool enum_converter<metternich::forestation_type>::initialized = enum_converter::initialize();

}
