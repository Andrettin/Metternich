#pragma once

#include "database/data_entry_history.h"
#include "domain/culture_container.h"
#include "population/population_group_map.h"
#include "species/phenotype_container.h"
#include "util/centesimal_int.h"

Q_MOC_INCLUDE("domain/country.h")
Q_MOC_INCLUDE("religion/religion.h")

namespace metternich {

class country;
class culture;
class province;
class religion;

class province_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner MEMBER owner)
	Q_PROPERTY(metternich::religion* religion MEMBER religion)
	Q_PROPERTY(int population READ get_population WRITE set_population)
	Q_PROPERTY(archimedes::centesimal_int literacy_rate MEMBER literacy_rate READ get_literacy_rate)

public:
	explicit province_history(const province *province) : province(province)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	const country *get_owner() const
	{
		return this->owner;
	}

	const culture_map<int64_t> &get_culture_weights() const
	{
		return this->culture_weights;
	}

	void set_culture_weights(const culture_map<int64_t> &weights)
	{
		this->culture_weights = weights;
	}

	const culture *get_main_culture() const;

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	const phenotype_map<int64_t> &get_phenotype_weights() const
	{
		return this->phenotype_weights;
	}

	void set_phenotype_weights(const phenotype_map<int64_t> &weights)
	{
		this->phenotype_weights = weights;
	}

	std::vector<const phenotype *> get_weighted_phenotypes_for_culture(const metternich::culture *culture) const;

	int get_population() const
	{
		static const population_group_key group_key;

		return this->get_group_population(group_key);
	}

	void set_population(const int population)
	{
		static const population_group_key group_key;

		this->set_group_population(group_key, population);
	}

	const population_group_map<int> &get_population_groups() const
	{
		return this->population_groups;
	}

	int get_group_population(const population_group_key &group_key) const
	{
		const auto find_iterator = this->population_groups.find(group_key);
		if (find_iterator != this->population_groups.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_group_population(const population_group_key &group_key, const int population)
	{
		if (population == 0) {
			this->population_groups.erase(group_key);
		} else {
			this->population_groups[group_key] = population;
		}
	}

	const population_group_map<int> &get_lower_bound_population_groups() const
	{
		return this->lower_bound_population_groups;
	}

	int get_lower_bound_group_population(const population_group_key &group_key) const
	{
		const auto find_iterator = this->lower_bound_population_groups.find(group_key);
		if (find_iterator != this->lower_bound_population_groups.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void initialize_population();
	void distribute_population();

	const centesimal_int &get_literacy_rate() const
	{
		return this->literacy_rate;
	}

	void set_literacy_rate(const centesimal_int &literacy_rate)
	{
		this->literacy_rate = literacy_rate;
	}

private:
	const metternich::province *province = nullptr;
	country *owner = nullptr;
	culture_map<int64_t> culture_weights;
	metternich::religion *religion = nullptr;
	phenotype_map<int64_t> phenotype_weights;
	population_group_map<int> population_groups;
	population_group_map<int> lower_bound_population_groups;
	centesimal_int literacy_rate;
};

}
