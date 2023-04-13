#include "metternich.h"

#include "technology/technology_category.h"

namespace archimedes {

template class enum_converter<metternich::technology_category>;

template <>
const std::string enum_converter<metternich::technology_category>::property_class_identifier = "metternich::technology_category";

template <>
const std::map<std::string, metternich::technology_category> enum_converter<metternich::technology_category>::string_to_enum_map = {
	{ "none", metternich::technology_category::none },
	{ "gathering", metternich::technology_category::gathering },
	{ "industry", metternich::technology_category::industry },
	{ "army", metternich::technology_category::army },
	{ "navy", metternich::technology_category::navy },
	{ "finance", metternich::technology_category::finance },
	{ "culture", metternich::technology_category::culture }
};

template <>
const bool enum_converter<metternich::technology_category>::initialized = enum_converter::initialize();

}
