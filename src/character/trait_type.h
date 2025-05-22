#pragma once

namespace metternich {

enum class trait_type {
	ruler,
	background,
	personality,
	expertise
};

}

Q_DECLARE_METATYPE(metternich::trait_type)
