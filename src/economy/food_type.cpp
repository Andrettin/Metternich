#include "metternich.h"

#include "economy/food_type.h"

namespace archimedes {

template class enum_converter<metternich::food_type>;

template <>
const std::string enum_converter<metternich::food_type>::property_class_identifier = "metternich::food_type";

template <>
const std::map<std::string, metternich::food_type> enum_converter<metternich::food_type>::string_to_enum_map = {
	{ "none", metternich::food_type::none },
	{ "starch", metternich::food_type::starch },
	{ "meat", metternich::food_type::meat },
	{ "fruit", metternich::food_type::fruit }
};

template <>
const bool enum_converter<metternich::food_type>::initialized = enum_converter::initialize();

}
