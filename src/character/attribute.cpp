#include "metternich.h"

#include "character/attribute.h"

namespace archimedes {

template class enum_converter<metternich::attribute>;

template <>
const std::string enum_converter<metternich::attribute>::property_class_identifier = "metternich::attribute";

template <>
const std::map<std::string, metternich::attribute> enum_converter<metternich::attribute>::string_to_enum_map = {
	{ "none", metternich::attribute::none },
	{ "skill", metternich::attribute::skill },
	{ "prowess", metternich::attribute::prowess },
	{ "vitality", metternich::attribute::vitality }
};

template <>
const bool enum_converter<metternich::attribute>::initialized = enum_converter::initialize();

}
