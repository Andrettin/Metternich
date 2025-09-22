#pragma once

namespace metternich {

enum class site_type {
	none,
	holding,
	resource,
	dungeon,
	habitable_world,
	celestial_body
};

}

Q_DECLARE_METATYPE(metternich::site_type)
