#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class region;

class region_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(int population MEMBER population READ get_population)

public:
	explicit region_history(const metternich::region *region) : region(region)
	{
	}

	int get_population() const
	{
		return this->population;
	}

	void distribute_population();
	void distribute_population_groups();

private:
	const metternich::region *region = nullptr;
	int population = 0;
};

}
