#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class region;

class region_history final : public data_entry_history
{
	Q_OBJECT

public:
	explicit region_history(const metternich::region *region) : region(region)
	{
	}

private:
	const metternich::region *region = nullptr;
};

}
