#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class trait_type {
	none,
	expertise,
	background,
	personality,
	weapon,
	armor,
	trinket
};

}

extern template class archimedes::enum_converter<metternich::trait_type>;

Q_DECLARE_METATYPE(metternich::trait_type)
