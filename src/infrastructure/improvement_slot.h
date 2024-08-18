#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class improvement_slot {
	none,
	main,
	resource,
	depot,
	port
};

}

extern template class archimedes::enum_converter<metternich::improvement_slot>;

Q_DECLARE_METATYPE(metternich::improvement_slot)
