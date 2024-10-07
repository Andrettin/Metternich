#include "metternich.h"

#include "character/character_attribute.h"

namespace archimedes {

template class enum_converter<metternich::character_attribute>;

template <>
const std::string enum_converter<metternich::character_attribute>::property_class_identifier = "metternich::character_attribute";

template <>
const std::map<std::string, metternich::character_attribute> enum_converter<metternich::character_attribute>::string_to_enum_map = {
	{ "none", metternich::character_attribute::none },
	{ "diplomacy", metternich::character_attribute::diplomacy },
	{ "martial", metternich::character_attribute::martial },
	{ "stewardship", metternich::character_attribute::stewardship },
	{ "intrigue", metternich::character_attribute::intrigue },
	{ "learning", metternich::character_attribute::learning }
};

template <>
const bool enum_converter<metternich::character_attribute>::initialized = enum_converter::initialize();

}
