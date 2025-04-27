#pragma once

namespace metternich {

enum class improvement_slot {
	none,
	main,
	resource,
	depot,
	port
};

}

Q_DECLARE_METATYPE(metternich::improvement_slot)
