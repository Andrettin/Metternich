#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class elevation_type {
	none,
	water,
	flatlands,
	hills,
	mountains
};

}

extern template class archimedes::enum_converter<metternich::elevation_type>;

Q_DECLARE_METATYPE(metternich::elevation_type)
