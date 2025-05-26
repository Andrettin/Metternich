#pragma once

namespace metternich {

enum class temperature_type {
	none,
	frozen,
	cold,
	temperate,
	tropical
};

}

Q_DECLARE_METATYPE(metternich::temperature_type)
