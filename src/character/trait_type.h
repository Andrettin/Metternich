#pragma once

namespace metternich {

enum class trait_type {
	background,
	personality,
	expertise,
	ruler,
	advisor,
	governor
};

}

Q_DECLARE_METATYPE(metternich::trait_type)
