#pragma once

namespace metternich {

enum class trait_type {
	none,
	ruler,
	background,
	personality,
	expertise
};

}

Q_DECLARE_METATYPE(metternich::trait_type)
