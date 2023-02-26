#include "metternich.h"

#include "unit/military_unit_domain.h"

namespace archimedes {

template class enum_converter<metternich::military_unit_domain>;

template <>
const std::string enum_converter<metternich::military_unit_domain>::property_class_identifier = "metternich::military_unit_domain";

template <>
const std::map<std::string, metternich::military_unit_domain> enum_converter<metternich::military_unit_domain>::string_to_enum_map = {
	{ "none", metternich::military_unit_domain::none },
	{ "land", metternich::military_unit_domain::land },
	{ "water", metternich::military_unit_domain::water },
	{ "air", metternich::military_unit_domain::air }
};

template <>
const bool enum_converter<metternich::military_unit_domain>::initialized = enum_converter::initialize();

}
