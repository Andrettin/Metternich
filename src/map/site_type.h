#pragma once

namespace metternich {

enum class site_type {
	none,
	holding,
	resource,
	habitable_world,
	celestial_body
};

}

Q_DECLARE_METATYPE(metternich::site_type)
