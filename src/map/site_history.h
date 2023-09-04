#pragma once

#include "database/data_entry_history.h"
#include "infrastructure/building_slot_type_container.h"
#include "population/population_group_map.h"

namespace metternich {

class building_type;
class improvement;
class settlement_type;
class site;
class wonder;

class site_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::improvement* improvement MEMBER improvement)
	Q_PROPERTY(metternich::settlement_type* settlement_type MEMBER settlement_type)
	Q_PROPERTY(bool resource_discovered MEMBER resource_discovered READ is_resource_discovered)
	Q_PROPERTY(bool developed MEMBER developed)
	Q_PROPERTY(int population READ get_population WRITE set_population)

public:
	explicit site_history(const metternich::site *site) : site(site)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	bool is_resource_discovered() const
	{
		return this->resource_discovered;
	}

	bool is_developed() const
	{
		return this->developed || this->get_improvement() != nullptr || this->get_settlement_type() != nullptr || !this->get_buildings().empty() || !this->get_wonders().empty() || !this->get_population_groups().empty();
	}

	const metternich::improvement *get_improvement() const
	{
		return this->improvement;
	}

	const metternich::settlement_type *get_settlement_type() const
	{
		return this->settlement_type;
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

	const building_slot_type_map<const wonder *> &get_wonders() const
	{
		return this->wonders;
	}

	const wonder *get_wonder(const building_slot_type *slot_type) const
	{
		const auto find_iterator = this->wonders.find(slot_type);
		if (find_iterator != this->wonders.end()) {
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
	bool resource_discovered = false;
	bool developed = false;
	metternich::improvement *improvement = nullptr;
	metternich::settlement_type *settlement_type = nullptr;
	building_slot_type_map<const building_type *> buildings;
	building_slot_type_map<const wonder *> wonders;
	population_group_map<int> population_groups;
};

}
