#include "metternich.h"

#include "script/special_target_type.h"

namespace archimedes {

template class enum_converter<metternich::special_target_type>;

template <>
const std::string enum_converter<metternich::special_target_type>::property_class_identifier = "metternich::special_target_type";

template <>
const std::map<std::string, metternich::special_target_type> enum_converter<metternich::special_target_type>::string_to_enum_map = {
	{ "root", metternich::special_target_type::root },
	{ "source", metternich::special_target_type::source },
	{ "previous", metternich::special_target_type::previous }
};

template <>
const bool enum_converter<metternich::special_target_type>::initialized = enum_converter::initialize();

}
