#pragma once

namespace metternich {

enum class elevation_type {
	none,
	water,
	flatlands,
	hills,
	mountains
};

}

Q_DECLARE_METATYPE(metternich::elevation_type)
