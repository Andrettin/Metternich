#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class attribute {
	none,
	diplomacy,
	martial,
	stewardship,
	intrigue,
	learning
};

}

extern template class archimedes::enum_converter<metternich::attribute>;

Q_DECLARE_METATYPE(metternich::attribute)
