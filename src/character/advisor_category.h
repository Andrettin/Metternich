#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class advisor_category {
	none,
	trade,
	exploration,
	military,
	political,
	religious
};

}

extern template class archimedes::enum_converter<metternich::advisor_category>;

Q_DECLARE_METATYPE(metternich::advisor_category)
