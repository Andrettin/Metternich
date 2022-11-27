#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class cultural_group_rank {
	none,
	infragroup,
	subgroup,
	group,
	supergroup
};

}

extern template class archimedes::enum_converter<metternich::cultural_group_rank>;

Q_DECLARE_METATYPE(metternich::cultural_group_rank)
