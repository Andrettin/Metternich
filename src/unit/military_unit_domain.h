#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_domain {
	none,
	land,
	water,
	air
};

}

extern template class archimedes::enum_converter<metternich::military_unit_domain>;

Q_DECLARE_METATYPE(metternich::military_unit_domain)
