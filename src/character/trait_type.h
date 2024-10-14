#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class trait_type {
	none,
	ruler,
	background,
	personality,
	expertise
};

}

extern template class archimedes::enum_converter<metternich::trait_type>;

Q_DECLARE_METATYPE(metternich::trait_type)
