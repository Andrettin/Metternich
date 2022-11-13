#pragma once

#include "database/data_entry_history.h"
#include "population/population_group_map.h"

namespace metternich {

class country;
class culture;
class province;

class province_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner MEMBER owner)
	Q_PROPERTY(metternich::culture* culture MEMBER culture)
	Q_PROPERTY(int population READ get_population WRITE set_population)

public:
	explicit province_history(const province *province) : province(province)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const country *get_owner() const
	{
		return this->owner;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

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

	void set_lower_bound_group_population(const population_group_key &group_key, const int population)
	{
		if (population == 0) {
			this->lower_bound_population_groups.erase(group_key);
		} else {
			this->lower_bound_population_groups[group_key] = population;
		}
	}

	void change_lower_bound_group_population(const population_group_key &group_key, const int change)
	{
		this->set_lower_bound_group_population(group_key, this->get_lower_bound_group_population(group_key) + change);
	}

	void initialize_population()
	{
		for (const auto &[group_key, lower_bound_population] : this->lower_bound_population_groups) {
			if (lower_bound_population > this->get_group_population(group_key)) {
				this->set_group_population(group_key, lower_bound_population);
			}
		}

		for (auto it = this->population_groups.begin(); it != this->population_groups.end(); ++it) {
			const population_group_key &group_key = it->first;
			const int population = it->second;

			auto other_it = it;
			++other_it;
			for (; other_it != this->population_groups.end(); ++other_it) {
				const population_group_key &other_group_key = other_it->first;
				int &other_population = other_it->second;

				if (other_group_key.contains(group_key)) {
					other_population -= population;
				}
			}
		}
	}

private:
	const metternich::province *province = nullptr;
	country *owner = nullptr;
	metternich::culture *culture = nullptr;
	population_group_map<int> population_groups;
	population_group_map<int> lower_bound_population_groups;
};

}
