#include "metternich.h"

#include "character/advisor_category.h"

namespace archimedes {

template class enum_converter<metternich::advisor_category>;

template <>
const std::string enum_converter<metternich::advisor_category>::property_class_identifier = "metternich::advisor_category";

template <>
const std::map<std::string, metternich::advisor_category> enum_converter<metternich::advisor_category>::string_to_enum_map = {
	{ "none", metternich::advisor_category::none },
	{ "trade", metternich::advisor_category::trade },
	{ "exploration", metternich::advisor_category::exploration },
	{ "military", metternich::advisor_category::military },
	{ "political", metternich::advisor_category::political },
	{ "religious", metternich::advisor_category::religious }
};

template <>
const bool enum_converter<metternich::advisor_category>::initialized = enum_converter::initialize();

}
