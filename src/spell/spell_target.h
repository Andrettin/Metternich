#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class spell_target {
	none,
	enemy,
	ally
};

}

extern template class archimedes::enum_converter<metternich::spell_target>;

Q_DECLARE_METATYPE(metternich::spell_target)
