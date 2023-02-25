#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class special_target_type {
	root,
	source,
	previous
};

}

extern template class archimedes::enum_converter<metternich::special_target_type>;

Q_DECLARE_METATYPE(metternich::special_target_type)
