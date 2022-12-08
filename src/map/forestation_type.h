#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class forestation_type {
	none,
	forest
};

}

extern template class archimedes::enum_converter<metternich::forestation_type>;

Q_DECLARE_METATYPE(metternich::forestation_type)
