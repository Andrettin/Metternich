#include "metternich.h"

#include "character/office_type.h"

namespace archimedes {

template class enum_converter<metternich::office_type>;

template <>
const std::string enum_converter<metternich::office_type>::property_class_identifier = "metternich::office_type";

template <>
const std::map<std::string, metternich::office_type> enum_converter<metternich::office_type>::string_to_enum_map = {
	{ "none", metternich::office_type::none },
	{ "country", metternich::office_type::country },
	{ "province", metternich::office_type::province }
};

template <>
const bool enum_converter<metternich::office_type>::initialized = enum_converter::initialize();

}
