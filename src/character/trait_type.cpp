#include "metternich.h"

#include "character/trait_type.h"

namespace archimedes {

template class enum_converter<metternich::trait_type>;

template <>
const std::string enum_converter<metternich::trait_type>::property_class_identifier = "metternich::trait_type";

template <>
const std::map<std::string, metternich::trait_type> enum_converter<metternich::trait_type>::string_to_enum_map = {
	{ "none", metternich::trait_type::none },
	{ "background", metternich::trait_type::background },
	{ "personality", metternich::trait_type::personality }
};

template <>
const bool enum_converter<metternich::trait_type>::initialized = enum_converter::initialize();

}
