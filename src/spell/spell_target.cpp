#include "metternich.h"

#include "spell/spell_target.h"

namespace archimedes {

template class enum_converter<metternich::spell_target>;

template <>
const std::string enum_converter<metternich::spell_target>::property_class_identifier = "metternich::spell_target";

template <>
const std::map<std::string, metternich::spell_target> enum_converter<metternich::spell_target>::string_to_enum_map = {
	{ "none", metternich::spell_target::none },
	{ "enemy", metternich::spell_target::enemy },
	{ "ally", metternich::spell_target::ally }
};

template <>
const bool enum_converter<metternich::spell_target>::initialized = enum_converter::initialize();

}
