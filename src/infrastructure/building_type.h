#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "util/fractional_int.h"

Q_MOC_INCLUDE("country/cultural_group.h")
Q_MOC_INCLUDE("country/culture.h")
Q_MOC_INCLUDE("infrastructure/building_class.h")
Q_MOC_INCLUDE("population/profession.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class building_class;
class building_slot_type;
class country;
class cultural_group;
class culture;
class icon;
class population_unit;
class portrait;
class production_type;
class profession;
class province;
class settlement_type;
class site;
class technology;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class factor;

template <typename scope_type>
class modifier;

class building_type final : public named_data_entry, public data_type<building_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_class* building_class MEMBER building_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(bool provincial MEMBER provincial READ is_provincial NOTIFY changed)
	Q_PROPERTY(const metternich::profession* employment_profession MEMBER employment_profession READ get_employment_profession NOTIFY changed)
	Q_PROPERTY(int employment_capacity MEMBER employment_capacity READ get_employment_capacity NOTIFY changed)
	Q_PROPERTY(QVariantList production_types READ get_production_types_qvariant_list NOTIFY changed)
	Q_PROPERTY(int base_capacity MEMBER base_capacity READ get_base_capacity NOTIFY changed)
	Q_PROPERTY(int capacity_increment MEMBER capacity_increment READ get_capacity_increment NOTIFY changed)
	Q_PROPERTY(bool capitol MEMBER capitol READ is_capitol NOTIFY changed)
	Q_PROPERTY(bool provincial_capitol MEMBER provincial_capitol READ is_provincial_capitol NOTIFY changed)
	Q_PROPERTY(bool warehouse MEMBER warehouse READ is_warehouse NOTIFY changed)
	Q_PROPERTY(bool free_in_capital MEMBER free_in_capital READ is_free_in_capital NOTIFY changed)
	Q_PROPERTY(bool free_on_start MEMBER free_on_start READ is_free_on_start NOTIFY changed)
	Q_PROPERTY(bool capital_only MEMBER capital_only READ is_capital_only NOTIFY changed)
	Q_PROPERTY(bool provincial_capital_only MEMBER provincial_capital_only READ is_provincial_capital_only NOTIFY changed)
	Q_PROPERTY(bool wonder_only MEMBER wonder_only READ is_wonder_only NOTIFY changed)
	Q_PROPERTY(bool expandable MEMBER expandable READ is_expandable NOTIFY changed)
	Q_PROPERTY(int max_level MEMBER max_level READ get_max_level NOTIFY changed)
	Q_PROPERTY(int fort_level MEMBER fort_level READ get_fort_level NOTIFY changed)
	Q_PROPERTY(metternich::building_type* required_building MEMBER required_building NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(int wealth_cost MEMBER wealth_cost READ get_wealth_cost NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "building_type";
	static constexpr const char property_class_identifier[] = "metternich::building_type*";
	static constexpr const char database_folder[] = "building_types";

public:
	explicit building_type(const std::string &identifier);
	~building_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const building_class *get_building_class() const
	{
		return this->building_class;
	}

	const building_slot_type *get_slot_type() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::cultural_group *get_cultural_group() const
	{
		return this->cultural_group;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_provincial() const
	{
		return this->provincial;
	}

	int get_level() const
	{
		return this->level;
	}

	void calculate_level();

	const std::vector<const settlement_type *> &get_settlement_types() const
	{
		return this->settlement_types;
	}

	const profession *get_employment_profession() const
	{
		return this->employment_profession;
	}

	int get_employment_capacity() const
	{
		return this->employment_capacity;
	}

	const std::vector<const production_type *> &get_production_types() const
	{
		return this->production_types;
	}

	QVariantList get_production_types_qvariant_list() const;

	int get_base_capacity() const
	{
		return this->base_capacity;
	}

	int get_capacity_increment() const
	{
		return this->capacity_increment;
	}

	bool is_capitol() const
	{
		return this->capitol;
	}

	bool is_provincial_capitol() const
	{
		return this->provincial_capitol;
	}

	bool is_warehouse() const
	{
		return this->warehouse;
	}

	bool is_free_in_capital() const
	{
		return this->free_in_capital;
	}

	bool is_free_on_start() const
	{
		return this->free_on_start;
	}

	bool is_capital_only() const
	{
		return this->capital_only;
	}

	bool is_provincial_capital_only() const
	{
		return this->provincial_capital_only;
	}

	bool is_wonder_only() const
	{
		return this->wonder_only;
	}

	bool is_expandable() const
	{
		return this->expandable;
	}

	int get_max_level() const
	{
		return this->max_level;
	}

	int get_fort_level() const
	{
		return this->fort_level;
	}

	const building_type *get_required_building() const
	{
		return this->required_building;
	}

	bool is_any_required_building(const building_type *building) const
	{
		if (building == nullptr) {
			return false;
		}

		if (this->get_required_building() == nullptr) {
			return false;
		}

		if (building == this->get_required_building()) {
			return true;
		}

		return this->get_required_building()->is_any_required_building(building);
	}

	const std::vector<const building_type *> &get_requiring_buildings() const
	{
		return this->requiring_buildings;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	int get_wealth_cost() const
	{
		return this->wealth_cost;
	}

	int get_wealth_cost_for_country(const country *country) const;

	const commodity_map<int> &get_commodity_costs() const
	{
		return this->commodity_costs;
	}

	commodity_map<int> get_commodity_costs_for_country(const country *country) const;

	const factor<country> *get_cost_factor() const
	{
		return this->cost_factor.get();
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const and_condition<site> *get_settlement_conditions() const
	{
		return this->settlement_conditions.get();
	}

	const condition<province> *get_province_conditions() const
	{
		return this->province_conditions.get();
	}

	const condition<site> *get_build_conditions() const
	{
		return this->build_conditions.get();
	}

	const modifier<const site> *get_settlement_modifier() const
	{
		return this->settlement_modifier.get();
	}

	const modifier<const province> *get_province_modifier() const
	{
		return this->province_modifier.get();
	}

	const modifier<const country> *get_country_modifier() const
	{
		return this->country_modifier.get();
	}

	const modifier<const country> *get_stackable_country_modifier() const
	{
		return this->stackable_country_modifier.get();
	}

	Q_INVOKABLE QString get_effects_string(metternich::site *site) const;

	const effect_list<const site> *get_effects() const
	{
		return this->effects.get();
	}

signals:
	void changed();

private:
	building_class *building_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::portrait *portrait = nullptr;
	metternich::icon *icon = nullptr;
	bool provincial = false;
	int level = 0;
	std::vector<const settlement_type *> settlement_types;
	const profession *employment_profession = nullptr;
	int employment_capacity = 0;
	std::vector<const production_type *> production_types;
	int base_capacity = 0;
	int capacity_increment = 0;
	bool capitol = false;
	bool provincial_capitol = false;
	bool warehouse = false;
	bool free_in_capital = false;
	bool free_on_start = false;
	bool capital_only = false;
	bool provincial_capital_only = false;
	bool wonder_only = false;
	bool expandable = false;
	int max_level = 0;
	int fort_level = 0;
	building_type *required_building = nullptr;
	std::vector<const building_type *> requiring_buildings; //buildings which require this one
	technology *required_technology = nullptr;
	int wealth_cost = 0;
	commodity_map<int> commodity_costs;
	std::unique_ptr<const factor<country>> cost_factor;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<and_condition<site>> settlement_conditions;
	std::unique_ptr<const condition<province>> province_conditions;
	std::unique_ptr<const condition<site>> build_conditions;
	std::unique_ptr<modifier<const site>> settlement_modifier;
	std::unique_ptr<modifier<const province>> province_modifier;
	std::unique_ptr<modifier<const country>> country_modifier;
	std::unique_ptr<modifier<const country>> stackable_country_modifier;
	std::unique_ptr<effect_list<const site>> effects;
};

}
