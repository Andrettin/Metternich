#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_category {
	none,
	beasts,
	colossal_beasts,
	flying_beasts,
	colossal_flying_beasts,
	militia,
	light_infantry,
	regular_infantry,
	heavy_infantry,
	bowmen,
	light_cavalry,
	heavy_cavalry,
	spear_cavalry,
	light_artillery,
	heavy_artillery,
	combat_engineers,
	general,
	light_warship,
	heavy_warship
};

}

extern template class archimedes::enum_converter<metternich::military_unit_category>;

Q_DECLARE_METATYPE(metternich::military_unit_category)
