#pragma once

#include "culture/culture_container.h"
#include "database/data_entry_history.h"
#include "population/population_group_map.h"
#include "species/phenotype_container.h"
#include "util/decimillesimal_int.h"

namespace metternich {

class region;
class technology;

class region_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(qint64 population READ get_population WRITE set_population)
	Q_PROPERTY(archimedes::decimillesimal_int literacy_rate MEMBER literacy_rate READ get_literacy_rate)
	Q_PROPERTY(std::vector<const metternich::technology *> technologies READ get_technologies)

public:
	explicit region_history(const metternich::region *region) : region(region)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope, const QDate &date) override;

	const culture_map<int64_t> &get_culture_weights() const
	{
		return this->culture_weights;
	}

	const phenotype_map<int64_t> &get_phenotype_weights() const
	{
		return this->phenotype_weights;
	}

	int64_t get_population() const
	{
		static const population_group_key key;

		const auto find_iterator = this->population_groups.find(key);
		if (find_iterator != this->population_groups.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_population(const int64_t population)
	{
		static const population_group_key key;

		if (population == 0) {
			this->population_groups.erase(key);
		} else {
			this->population_groups[key] = population;
		}
	}

	void distribute_population();

	const decimillesimal_int &get_literacy_rate() const
	{
		return this->literacy_rate;
	}

	const std::vector<const technology *> &get_technologies() const
	{
		return this->technologies;
	}

	Q_INVOKABLE void add_technology(const technology *technology)
	{
		this->technologies.push_back(technology);
	}

	Q_INVOKABLE void remove_technology(const technology *technology)
	{
		std::erase(this->technologies, technology);
	}

	void apply_to_provinces() const;

private:
	const metternich::region *region = nullptr;
	culture_map<int64_t> culture_weights;
	phenotype_map<int64_t> phenotype_weights;
	population_group_map<int64_t> population_groups;
	decimillesimal_int literacy_rate;
	std::vector<const technology *> technologies;
};

}
