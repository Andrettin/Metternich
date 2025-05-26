#pragma once

namespace metternich {

enum class cultural_group_rank {
	none,
	infragroup,
	subgroup,
	group,
	supergroup
};

}

Q_DECLARE_METATYPE(metternich::cultural_group_rank)
