#pragma once

namespace metternich {

enum class site_tier {
	none = 0,
	barony = 1,
	viscounty = 2,
	county = 3,
	marquisate = 4,
	duchy = 5
};

}

Q_DECLARE_METATYPE(metternich::site_tier)
