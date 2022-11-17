#pragma once

#include "database/data_entry_history.h"
#include "infrastructure/building_slot_type_container.h"
#include "population/population_group_map.h"

namespace metternich {

class improvement;
class site;

class site_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::improvement* improvement MEMBER improvement)
	Q_PROPERTY(int population READ get_population WRITE set_population)

public:
	explicit site_history(const metternich::site *site) : site(site)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	const metternich::improvement *get_improvement() const
	{
		return this->improvement;
	}

	const building_slot_type_map<const building_type *> &get_buildings() const
	{
		return this->buildings;
	}

	const building_type *get_building(const building_slot_type *slot_type) const
	{
		const auto find_iterator = this->buildings.find(slot_type);
		if (find_iterator != this->buildings.end()) {
			return find_iterator->second;
		}

		return nullptr;
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

private:
	const metternich::site *site = nullptr;
	metternich::improvement *improvement = nullptr;
	building_slot_type_map<const building_type *> buildings;
	population_group_map<int> population_groups;
};

}
