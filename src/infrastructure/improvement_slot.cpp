#include "metternich.h"

#include "infrastructure/improvement_slot.h"

namespace archimedes {

template class enum_converter<metternich::improvement_slot>;

template <>
const std::string enum_converter<metternich::improvement_slot>::property_class_identifier = "metternich::improvement_slot";

template <>
const std::map<std::string, metternich::improvement_slot> enum_converter<metternich::improvement_slot>::string_to_enum_map = {
	{ "none", metternich::improvement_slot::none },
	{ "main", metternich::improvement_slot::main },
	{ "resource", metternich::improvement_slot::resource },
	{ "depot", metternich::improvement_slot::depot },
	{ "port", metternich::improvement_slot::port }
};

template <>
const bool enum_converter<metternich::improvement_slot>::initialized = enum_converter::initialize();

}
