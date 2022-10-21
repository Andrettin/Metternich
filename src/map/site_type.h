#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class site_type {
	none,
	settlement,
	terrain,
	resource
};

}

extern template class archimedes::enum_converter<metternich::site_type>;

Q_DECLARE_METATYPE(metternich::site_type)
