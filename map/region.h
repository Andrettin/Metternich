#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "util/qunique_ptr.h"

#include <QVariant>

#include <memory>
#include <vector>

namespace metternich {

class holding;
class holding_slot;
class landed_title;
class population_unit;
class province;
class technology;
class territory;
class wildlife_unit;
class world;

class region final : public data_entry, public data_type<region>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList holdings READ get_holdings_qvariant_list)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list)
	Q_PROPERTY(QVariantList worlds READ get_worlds_qvariant_list)
	Q_PROPERTY(QVariantList subregions READ get_subregions_qvariant_list)
	Q_PROPERTY(QVariantList technologies READ get_technologies_qvariant_list)

public:
	static constexpr const char *class_identifier = "region";
	static constexpr const char *database_folder = "regions";

	static std::set<std::string> get_database_dependencies();

public:
	region(const std::string &identifier);
	virtual ~region() override;

	virtual void initialize_history() override;

	const std::set<territory *> &get_territories() const
	{
		return this->territories;
	}

	void add_territory(territory *territory);
	void remove_territory(territory *territory);

	const std::set<province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;
	Q_INVOKABLE void add_province(province *province);
	Q_INVOKABLE void remove_province(province *province);

	const std::set<world *> &get_worlds() const
	{
		return this->worlds;
	}

	QVariantList get_worlds_qvariant_list() const;
	Q_INVOKABLE void add_world(world *world);
	Q_INVOKABLE void remove_world(world *world);

	QVariantList get_subregions_qvariant_list() const;

	Q_INVOKABLE void add_subregion(metternich::region *subregion)
	{
		this->subregions.insert(subregion);
		subregion->superregions.insert(this);

		for (province *province : subregion->get_provinces()) {
			this->add_province(province);
		}

		for (world *world : subregion->get_worlds()) {
			this->add_world(world);
		}

		for (holding_slot *holding_slot : subregion->holding_slots) {
			this->add_holding(holding_slot);
		}
	}

	Q_INVOKABLE void remove_subregion(metternich::region *subregion)
	{
		this->subregions.erase(subregion);
		subregion->superregions.erase(this);

		for (province *province : subregion->get_provinces()) {
			this->remove_province(province);
		}

		for (world *world : subregion->get_worlds()) {
			this->remove_world(world);
		}

		for (holding_slot *holding_slot : subregion->holding_slots) {
			this->remove_holding(holding_slot);
		}
	}

	std::vector<holding *> get_holdings() const;

	QVariantList get_holdings_qvariant_list() const;

	Q_INVOKABLE void add_holding(holding_slot *holding_slot)
	{
		this->holding_slots.insert(holding_slot);

		for (region *superregion : this->superregions) {
			superregion->add_holding(holding_slot);
		}
	}

	Q_INVOKABLE void remove_holding(holding_slot *holding_slot)
	{
		this->holding_slots.erase(holding_slot);

		for (region *superregion : this->superregions) {
			superregion->remove_holding(holding_slot);
		}
	}

	QVariantList get_technologies_qvariant_list() const;

	Q_INVOKABLE void add_technology(technology *technology)
	{
		this->technologies.insert(technology);
	}

	Q_INVOKABLE void remove_technology(technology *technology)
	{
		this->technologies.erase(technology);
	}

	const std::vector<qunique_ptr<population_unit>> &get_population_units() const
	{
		return this->population_units;
	}

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);

	const std::vector<qunique_ptr<wildlife_unit>> &get_wildlife_units() const
	{
		return this->wildlife_units;
	}

	void add_wildlife_unit(qunique_ptr<wildlife_unit> &&wildlife_unit);

signals:
	void provinces_changed();
	void worlds_changed();

private:
	std::set<province *> provinces;
	std::set<world *> worlds;
	std::set<territory *> territories;
	std::set<holding_slot *> holding_slots; //the slots for the holdings contained by this region
	std::set<region *> subregions; //subregions of this region
	std::set<region *> superregions; //regions for which this region is a subregion
	std::set<technology *> technologies; //technologies to be applied to this region's territories
	std::vector<qunique_ptr<population_unit>> population_units; //population units set for this region in history, used during initialization to generate population units in the region's settlements
	std::vector<qunique_ptr<wildlife_unit>> wildlife_units; //wildlife units set for this region in history, used during initialization to generate wildlife units in the region's territories
};

}
