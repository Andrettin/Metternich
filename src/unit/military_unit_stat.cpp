#include "metternich.h"

#include "unit/military_unit_stat.h"

namespace archimedes {

template class enum_converter<metternich::military_unit_stat>;

template <>
const std::string enum_converter<metternich::military_unit_stat>::property_class_identifier = "metternich::military_unit_stat";

template <>
const std::map<std::string, metternich::military_unit_stat> enum_converter<metternich::military_unit_stat>::string_to_enum_map = {
	{ "firepower", metternich::military_unit_stat::firepower },
	{ "melee", metternich::military_unit_stat::melee },
	{ "shock", metternich::military_unit_stat::shock },
	{ "range", metternich::military_unit_stat::range },
	{ "defense", metternich::military_unit_stat::defense },
	{ "resistance", metternich::military_unit_stat::resistance },
	{ "morale_resistance", metternich::military_unit_stat::morale_resistance },
	{ "movement", metternich::military_unit_stat::movement }
};

template <>
const bool enum_converter<metternich::military_unit_stat>::initialized = enum_converter::initialize();

}
