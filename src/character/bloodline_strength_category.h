#pragma once

namespace metternich {

enum class bloodline_strength_category {
	none,
	tainted,
	minor,
	major,
	great,
	true_bloodline
};

}

Q_DECLARE_METATYPE(metternich::bloodline_strength_category)
