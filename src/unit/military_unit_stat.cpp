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
	{ "discipline", metternich::military_unit_stat::discipline },
	{ "movement", metternich::military_unit_stat::movement },
	{ "damage_bonus", metternich::military_unit_stat::damage_bonus },
	{ "bonus_vs_infantry", metternich::military_unit_stat::bonus_vs_infantry },
	{ "bonus_vs_cavalry", metternich::military_unit_stat::bonus_vs_cavalry },
	{ "bonus_vs_artillery", metternich::military_unit_stat::bonus_vs_artillery },
	{ "bonus_vs_fortifications", metternich::military_unit_stat::bonus_vs_fortifications },
	{ "ranged_defense_modifier", metternich::military_unit_stat::ranged_defense_modifier },
	{ "entrenchment_bonus_modifier", metternich::military_unit_stat::entrenchment_bonus_modifier },
	{ "desert_attack_modifier", metternich::military_unit_stat::desert_attack_modifier },
	{ "desert_defense_modifier", metternich::military_unit_stat::desert_defense_modifier },
	{ "forest_attack_modifier", metternich::military_unit_stat::forest_attack_modifier },
	{ "forest_defense_modifier", metternich::military_unit_stat::forest_defense_modifier },
	{ "hills_attack_modifier", metternich::military_unit_stat::hills_attack_modifier },
	{ "hills_defense_modifier", metternich::military_unit_stat::hills_defense_modifier },
	{ "mountains_attack_modifier", metternich::military_unit_stat::mountains_attack_modifier },
	{ "mountains_defense_modifier", metternich::military_unit_stat::mountains_defense_modifier },
	{ "wetland_attack_modifier", metternich::military_unit_stat::wetland_attack_modifier },
	{ "wetland_defense_modifier", metternich::military_unit_stat::wetland_defense_modifier }
};

template <>
const bool enum_converter<metternich::military_unit_stat>::initialized = enum_converter::initialize();

}
