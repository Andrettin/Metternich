#pragma once

namespace metternich {

enum class profession_profitability {
	none,
	marginal,
	fair,
	good,
	excellent
};

}

Q_DECLARE_METATYPE(metternich::profession_profitability)
