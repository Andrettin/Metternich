#pragma once

#include "database/data_entry_history.h"
#include "population/population_group_map.h"

namespace metternich {

class region;

class region_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(int population READ get_population WRITE set_population)

public:
	explicit region_history(const metternich::region *region) : region(region)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	int get_population() const
	{
		static const population_group_key key;

		const auto find_iterator = this->population_groups.find(key);
		if (find_iterator != this->population_groups.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_population(const int population)
	{
		static const population_group_key key;

		if (population == 0) {
			this->population_groups.erase(key);
		} else {
			this->population_groups[key] = population;
		}
	}

	void distribute_population();

private:
	const metternich::region *region = nullptr;
	population_group_map<int> population_groups;
};

}
