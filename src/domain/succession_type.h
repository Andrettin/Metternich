#pragma once

namespace metternich {

enum class succession_type {
	none,
	primogeniture,
	ultimogeniture,
	elective
};

}

Q_DECLARE_METATYPE(metternich::succession_type)
