#pragma once

namespace metternich {

enum class site_tier {
	none = 0,
	barony = 1,
	viscounty = 2,
	county = 3,
	marquisate = 4,
	bishopric = 4,
	duchy = 5,
	archbishopric = 5
};

}

Q_DECLARE_METATYPE(metternich::site_tier)
