#pragma once

namespace metternich {

enum class battle_resolution_type {
	none,
	crossed_swords,
	shield,
	pennant,

	count
};

}

Q_DECLARE_METATYPE(metternich::battle_resolution_type)
