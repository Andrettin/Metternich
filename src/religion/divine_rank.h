#pragma once

namespace metternich {

enum class divine_rank {
	none,
	quasi_deity,
	demigod,
	lesser_deity,
	intermediate_deity,
	greater_deity,
	overdeity
};

}

Q_DECLARE_METATYPE(metternich::divine_rank)
