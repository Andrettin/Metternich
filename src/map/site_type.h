#pragma once

namespace metternich {

enum class site_type {
	none,
	settlement,
	terrain,
	resource,
	habitable_world,
	celestial_body
};

}

Q_DECLARE_METATYPE(metternich::site_type)
