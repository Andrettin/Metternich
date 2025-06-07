#pragma once

namespace metternich {

enum class character_trait_type {
	background,
	personality,
	expertise,
	ruler,
	advisor,
	governor
};

}

Q_DECLARE_METATYPE(metternich::character_trait_type)
