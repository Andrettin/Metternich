#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class character_attribute {
	none,
	diplomacy,
	martial,
	stewardship,
	intrigue,
	learning
};

}

extern template class archimedes::enum_converter<metternich::character_attribute>;

Q_DECLARE_METATYPE(metternich::character_attribute)
