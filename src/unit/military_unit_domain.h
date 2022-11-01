#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class military_unit_domain {
	land,
	water
};

}

extern template class archimedes::enum_converter<metternich::military_unit_domain>;

Q_DECLARE_METATYPE(metternich::military_unit_domain)
