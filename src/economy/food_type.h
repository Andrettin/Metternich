#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class food_type {
	none,
	starch,
	meat,
	fruit
};

}

extern template class archimedes::enum_converter<metternich::food_type>;

Q_DECLARE_METATYPE(metternich::food_type)
