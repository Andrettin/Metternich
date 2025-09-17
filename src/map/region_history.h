#pragma once

#include "database/data_entry_history.h"
#include "domain/culture_container.h"
#include "population/population_group_map.h"
#include "species/phenotype_container.h"
#include "util/centesimal_int.h"

namespace metternich {

class region;

class region_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(int population READ get_population WRITE set_population)
	Q_PROPERTY(archimedes::centesimal_int literacy_rate MEMBER literacy_rate READ get_literacy_rate)

public:
	explicit region_history(const metternich::region *region) : region(region)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const culture_map<int64_t> &get_culture_weights() const
	{
		return this->culture_weights;
	}

	const phenotype_map<int64_t> &get_phenotype_weights() const
	{
		return this->phenotype_weights;
	}

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

	const centesimal_int &get_literacy_rate() const
	{
		return this->literacy_rate;
	}

private:
	const metternich::region *region = nullptr;
	culture_map<int64_t> culture_weights;
	phenotype_map<int64_t> phenotype_weights;
	population_group_map<int> population_groups;
	centesimal_int literacy_rate;
};

}
