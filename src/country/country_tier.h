#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class country_tier {
	none,
	barony,
	county,
	duchy,
	kingdom,
	empire
};

}

extern template class archimedes::enum_converter<metternich::country_tier>;

Q_DECLARE_METATYPE(metternich::country_tier)
