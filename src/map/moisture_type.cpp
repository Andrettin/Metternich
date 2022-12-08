#include "metternich.h"

#include "map/moisture_type.h"

namespace archimedes {

template class enum_converter<metternich::moisture_type>;

template <>
const std::string enum_converter<metternich::moisture_type>::property_class_identifier = "metternich::moisture_type";

template <>
const std::map<std::string, metternich::moisture_type> enum_converter<metternich::moisture_type>::string_to_enum_map = {
	{ "none", metternich::moisture_type::none },
	{ "arid", metternich::moisture_type::arid },
	{ "semi_arid", metternich::moisture_type::semi_arid },
	{ "dry", metternich::moisture_type::dry },
	{ "moist", metternich::moisture_type::moist },
	{ "wet", metternich::moisture_type::wet }
};

template <>
const bool enum_converter<metternich::moisture_type>::initialized = enum_converter::initialize();

}
