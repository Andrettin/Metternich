#pragma once

namespace metternich {

enum class affix_type {
	none,
	prefix,
	suffix,
	stem
};

}

Q_DECLARE_METATYPE(metternich::affix_type)
