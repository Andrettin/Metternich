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
	{ "movement", metternich::military_unit_stat::movement },
	{ "damage_bonus", metternich::military_unit_stat::damage_bonus },
	{ "bonus_vs_infantry", metternich::military_unit_stat::bonus_vs_infantry },
	{ "bonus_vs_cavalry", metternich::military_unit_stat::bonus_vs_cavalry },
	{ "bonus_vs_artillery", metternich::military_unit_stat::bonus_vs_artillery },
	{ "bonus_vs_fortifications", metternich::military_unit_stat::bonus_vs_fortifications },
	{ "ranged_defense_modifier", metternich::military_unit_stat::ranged_defense_modifier },
	{ "entrenchment_bonus_modifier", metternich::military_unit_stat::entrenchment_bonus_modifier }
};

template <>
const bool enum_converter<metternich::military_unit_stat>::initialized = enum_converter::initialize();

}
