#pragma once

namespace metternich {

enum class country_tier {
	none,
	barony,
	county,
	duchy,
	kingdom,
	empire
};

}

Q_DECLARE_METATYPE(metternich::country_tier)
