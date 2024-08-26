#include "metternich.h"

#include "country/tradition_category.h"

namespace archimedes {

template class enum_converter<metternich::tradition_category>;

template <>
const std::string enum_converter<metternich::tradition_category>::property_class_identifier = "metternich::tradition_category";

template <>
const std::map<std::string, metternich::tradition_category> enum_converter<metternich::tradition_category>::string_to_enum_map = {
	{ "none", metternich::tradition_category::none },
	{ "tradition", metternich::tradition_category::tradition },
	{ "belief", metternich::tradition_category::belief }
};

template <>
const bool enum_converter<metternich::tradition_category>::initialized = enum_converter::initialize();

}
