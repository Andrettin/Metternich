#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class office_type {
	none,
	country,
	province
};

}

extern template class archimedes::enum_converter<metternich::office_type>;

Q_DECLARE_METATYPE(metternich::office_type)
