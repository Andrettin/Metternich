#pragma once

namespace metternich {

enum class moisture_type {
	none,
	arid,
	semi_arid,
	dry,
	moist,
	wet
};

}

Q_DECLARE_METATYPE(metternich::moisture_type)
