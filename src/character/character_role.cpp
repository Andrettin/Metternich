#include "metternich.h"

#include "character/character_role.h"

namespace archimedes {

template class enum_converter<metternich::character_role>;

template <>
const std::string enum_converter<metternich::character_role>::property_class_identifier = "metternich::character_role";

template <>
const std::map<std::string, metternich::character_role> enum_converter<metternich::character_role>::string_to_enum_map = {
	{ "none", metternich::character_role::none },
	{ "ruler", metternich::character_role::ruler },
	{ "advisor", metternich::character_role::advisor },
	{ "leader", metternich::character_role::leader },
	{ "civilian", metternich::character_role::civilian }
};

template <>
const bool enum_converter<metternich::character_role>::initialized = enum_converter::initialize();

}
