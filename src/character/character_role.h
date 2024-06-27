#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class character_role {
	none,
	ruler,
	advisor,
	leader,
	civilian
};

}

extern template class archimedes::enum_converter<metternich::character_role>;

Q_DECLARE_METATYPE(metternich::character_role)
