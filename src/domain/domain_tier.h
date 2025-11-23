#pragma once

namespace metternich {

enum class domain_tier {
	none,
	barony,
	county,
	duchy,
	kingdom,
	empire
};

}

Q_DECLARE_METATYPE(metternich::domain_tier)
