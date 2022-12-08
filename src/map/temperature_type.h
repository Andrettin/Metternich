#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class temperature_type {
	none,
	frozen,
	cold,
	temperate,
	tropical
};

}

extern template class archimedes::enum_converter<metternich::temperature_type>;

Q_DECLARE_METATYPE(metternich::temperature_type)
