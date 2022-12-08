#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class moisture_type {
	none,
	arid,
	semi_arid,
	dry,
	moist,
	wet
};

}

extern template class archimedes::enum_converter<metternich::moisture_type>;

Q_DECLARE_METATYPE(metternich::moisture_type)
