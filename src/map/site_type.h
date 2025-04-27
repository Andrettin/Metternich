#pragma once

namespace metternich {

enum class site_type {
	none,
	settlement,
	terrain,
	resource
};

}

Q_DECLARE_METATYPE(metternich::site_type)
