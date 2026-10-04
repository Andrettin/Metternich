#pragma once

namespace metternich {

enum class character_class_type {
	base_class,
	subclass,
	prestige_class,
	epic_class
};

}

Q_DECLARE_METATYPE(metternich::character_class_type)
