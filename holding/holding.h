#pragma once

#include "database/data_entry.h"
#include "util/qunique_ptr.h"
#include "warfare/troop_type_map.h"

#include <QVariant>

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace metternich {

class building;
class building_slot;
class character;
class commodity;
class culture;
class employment;
class employment_type;
class holding_modifier;
class holding_slot;
class holding_type;
class landed_title;
class phenotype;
class population_type;
class population_unit;
class province;
class religion;
class terrain_type;
class territory;
class troop_type;
class world;

class holding final : public data_entry
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring NOTIFY name_changed)
	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY titled_name_changed)
	Q_PROPERTY(metternich::holding_slot* slot MEMBER slot READ get_slot CONSTANT)
	Q_PROPERTY(metternich::holding_type* type READ get_type NOTIFY type_changed)
	Q_PROPERTY(metternich::landed_title* barony READ get_barony CONSTANT)
	Q_PROPERTY(bool settlement READ is_settlement CONSTANT)
	Q_PROPERTY(QString portrait_path READ get_portrait_path_qstring NOTIFY portrait_path_changed)
	Q_PROPERTY(metternich::character* owner READ get_owner NOTIFY owner_changed)
	Q_PROPERTY(int population READ get_population WRITE set_population NOTIFY population_changed)
	Q_PROPERTY(int population_capacity READ get_population_capacity NOTIFY population_capacity_changed)
	Q_PROPERTY(int population_growth READ get_population_growth NOTIFY population_growth_changed)
	Q_PROPERTY(QVariantList population_units READ get_population_units_qvariant_list NOTIFY population_units_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list NOTIFY building_slots_changed)
	Q_PROPERTY(QVariantList buildings READ get_buildings_qvariant_list NOTIFY buildings_changed)
	Q_PROPERTY(metternich::building* under_construction_building READ get_under_construction_building NOTIFY under_construction_building_changed)
	Q_PROPERTY(int construction_days READ get_construction_days NOTIFY construction_days_changed)
	Q_PROPERTY(metternich::commodity* commodity READ get_commodity WRITE set_commodity NOTIFY commodity_changed)
	Q_PROPERTY(metternich::culture* culture READ get_culture WRITE set_culture NOTIFY culture_changed)
	Q_PROPERTY(metternich::religion* religion READ get_religion WRITE set_religion NOTIFY religion_changed)
	Q_PROPERTY(QVariantList levies READ get_levies_qvariant_list NOTIFY levies_changed)
	Q_PROPERTY(QVariantList troop_stats READ get_troop_stats_qvariant_list NOTIFY troop_stats_changed)
	Q_PROPERTY(bool selected READ is_selected WRITE set_selected NOTIFY selected_changed)

public:
	static holding *get_selected_holding()
	{
		return holding::selected_holding;
	}

private:
	static inline holding *selected_holding = nullptr;

public:
	holding(holding_slot *slot, holding_type *type);
	virtual ~holding() override;

	virtual void initialize_history() override;
	virtual void check_history() const override;

	void do_day();
	void do_month();
	void do_year();

	virtual std::string get_name() const override;

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	std::string get_type_name() const;
	std::string get_titled_name() const;

	QString get_titled_name_qstring() const
	{
		return QString::fromStdString(this->get_titled_name());
	}

	std::vector<std::vector<std::string>> get_tag_suffix_list_with_fallbacks() const;

	landed_title *get_barony() const;

	holding_type *get_type() const
	{
		return this->type;
	}

	void set_type(holding_type *type);

	holding_slot *get_slot() const
	{
		return this->slot;
	}

	bool is_settlement() const;

	const std::filesystem::path &get_portrait_path() const;

	QString get_portrait_path_qstring() const
	{
		return "file:///" + QString::fromStdString(this->get_portrait_path().string());
	}

	character *get_owner() const
	{
		return this->owner;
	}

	void set_owner(character *new_owner);

	territory *get_territory() const;
	province *get_province() const;
	world *get_world() const;

	landed_title *get_realm() const;

	const terrain_type *get_terrain() const;

	bool is_territory_capital() const;

	bool can_have_population_type(const population_type *population_type) const;
	population_type *get_equivalent_population_type(const population_type *population_type) const;

	const std::vector<qunique_ptr<population_unit>> &get_population_units() const
	{
		return this->population_units;
	}

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);
	population_unit *get_population_unit(const population_type *type, const culture *culture, const religion *religion, const phenotype *phenotype) const;
	void change_population_size(population_type *type, culture *culture, religion *religion, phenotype *phenotype, const int change);
	QVariantList get_population_units_qvariant_list() const;
	void sort_population_units();
	void remove_empty_population_units();
	void move_population_units_to(holding *other_holding);
	void update_population_types();

	int get_population() const
	{
		return this->population;
	}

	void set_population(const int population);

	void change_population(const int change)
	{
		this->set_population(this->get_population() + change);
	}

	void calculate_population();

	int get_base_population_capacity() const
	{
		return this->base_population_capacity;
	}

	void set_base_population_capacity(const int base_population_capacity)
	{
		if (base_population_capacity == this->get_base_population_capacity()) {
			return;
		}

		this->base_population_capacity = base_population_capacity;
		this->calculate_population_capacity();
	}

	void change_base_population_capacity(const int change)
	{
		this->set_base_population_capacity(this->get_base_population_capacity() + change);
	}

	int get_population_capacity_modifier() const
	{
		return this->population_capacity_modifier;
	}

	void set_population_capacity_modifier(const int population_capacity_modifier)
	{
		if (population_capacity_modifier == this->get_population_capacity_modifier()) {
			return;
		}

		this->population_capacity_modifier = population_capacity_modifier;
		this->calculate_population_capacity();
	}

	void change_population_capacity_modifier(const int change)
	{
		this->set_population_capacity_modifier(this->get_population_capacity_modifier() + change);
	}

	int get_population_capacity() const
	{
		return this->population_capacity;
	}

	void set_population_capacity(const int population_capacity)
	{
		if (population_capacity == this->get_population_capacity()) {
			return;
		}

		this->population_capacity = population_capacity;
		emit population_capacity_changed();
		this->calculate_population_growth(); //population growth depends on the population capacity
	}

	void calculate_population_capacity();

	int get_base_population_growth() const
	{
		return this->base_population_growth;
	}

	void set_base_population_growth(const int base_population_growth)
	{
		if (base_population_growth == this->get_base_population_growth()) {
			return;
		}

		this->base_population_growth = base_population_growth;
		this->calculate_population_growth();
	}

	void change_base_population_growth(const int change)
	{
		this->set_base_population_growth(this->get_base_population_growth() + change);
	}


	int get_population_growth() const
	{
		return this->population_growth;
	}

	void set_population_growth(const int population_growth)
	{
		if (population_growth == this->get_population_growth()) {
			return;
		}

		this->population_growth = population_growth;
		emit population_growth_changed();
	}

	void calculate_population_growth()
	{
		if (this->get_population() == 0) {
			this->set_population_growth(0);
			return;
		}

		int population_growth = this->get_base_population_growth();
		if (population_growth > 0 && this->get_population() >= this->get_population_capacity()) {
			population_growth = 0;
		}

		this->set_population_growth(population_growth);
	}

	void do_population_growth();
	void check_overpopulation();

	const std::map<population_type *, int> &get_population_per_type() const
	{
		return this->population_per_type;
	}

	int get_population_type_population(population_type *population_type) const
	{
		auto find_iterator = this->population_per_type.find(population_type);
		if (find_iterator == this->population_per_type.end()) {
			return 0;
		}

		return find_iterator->second;
	}

	const std::map<metternich::culture *, int> &get_population_per_culture() const
	{
		return this->population_per_culture;
	}

	int get_culture_population(metternich::culture *culture) const
	{
		auto find_iterator = this->population_per_culture.find(culture);
		if (find_iterator == this->population_per_culture.end()) {
			return 0;
		}

		return find_iterator->second;
	}

	const std::map<metternich::religion *, int> &get_population_per_religion() const
	{
		return this->population_per_religion;
	}

	int get_religion_population(metternich::religion *religion) const
	{
		auto find_iterator = this->population_per_religion.find(religion);
		if (find_iterator == this->population_per_religion.end()) {
			return 0;
		}

		return find_iterator->second;
	}

	void calculate_population_groups();

	std::vector<building_slot *> get_building_slots() const;
	QVariantList get_building_slots_qvariant_list() const;
	std::set<building *> get_buildings() const;
	QVariantList get_buildings_qvariant_list() const;
	bool has_building(building *building) const;
	Q_INVOKABLE void add_building(building *building);
	Q_INVOKABLE void remove_building(building *building);
	void calculate_building_slots();

	building *get_under_construction_building() const
	{
		return this->under_construction_building;
	}

	void set_under_construction_building(building *building);

	int get_construction_days() const
	{
		return this->construction_days;
	}

	void set_construction_days(const int construction_days)
	{
		if (construction_days == this->get_construction_days()) {
			return;
		}

		this->construction_days = construction_days;
		emit construction_days_changed();
	}

	void change_construction_days(const int change)
	{
		this->set_construction_days(this->get_construction_days() + change);
	}

	metternich::commodity *get_commodity() const
	{
		return this->commodity;
	}

	void set_commodity(commodity *commodity)
	{
		if (commodity == this->get_commodity()) {
			return;
		}

		this->commodity = commodity;
		emit commodity_changed();
	}

	metternich::culture *get_culture() const
	{
		return this->culture;
	}

	void set_culture(culture *culture)
	{
		if (culture == this->get_culture()) {
			return;
		}

		this->culture = culture;
		emit culture_changed();
	}

	metternich::religion *get_religion() const
	{
		return this->religion;
	}

	void set_religion(religion *religion)
	{
		if (religion == this->get_religion()) {
			return;
		}

		this->religion = religion;
		emit religion_changed();
	}

	int get_holding_size() const;

	const std::set<employment *> &get_employments() const
	{
		return this->employments;
	}

	void add_employment(employment *employment)
	{
		this->employments.insert(employment);
	}

	void remove_employment(employment *employment)
	{
		this->employments.erase(employment);
	}

	bool has_any_trade_route() const;
	bool has_any_active_trade_route() const;
	bool has_any_trade_route_land_connection() const;

	QVariantList get_levies_qvariant_list() const;

	int get_levy(troop_type *troop_type) const
	{
		auto find_iterator = this->levies.find(troop_type);
		if (find_iterator == this->levies.end()) {
			return 0;
		}

		return find_iterator->second;
	}

	void set_levy(troop_type *troop_type, const int levy)
	{
		if (levy == this->get_levy(troop_type)) {
			return;
		}

		if (levy == 0) {
			this->levies.erase(troop_type);
		} else {
			this->levies[troop_type] = levy;
		}

		emit levies_changed();
	}

	void change_levy(troop_type *troop_type, const int change)
	{
		this->set_levy(troop_type, this->get_levy(troop_type) + change);
	}

	QVariantList get_troop_stats_qvariant_list() const;

	int get_troop_attack(troop_type *troop_type) const;

	int get_troop_attack_modifier(troop_type *troop_type) const
	{
		auto find_iterator = this->troop_attack_modifiers.find(troop_type);
		if (find_iterator == this->troop_attack_modifiers.end()) {
			return 0;
		}

		return find_iterator->second;
	}

	void set_troop_attack_modifier(troop_type *troop_type, const int modifier)
	{
		if (modifier == this->get_troop_attack_modifier(troop_type)) {
			return;
		}

		if (modifier == 0) {
			this->troop_attack_modifiers.erase(troop_type);
		} else {
			this->troop_attack_modifiers[troop_type] = modifier;
		}

		emit troop_stats_changed();
	}

	void change_troop_attack_modifier(troop_type *troop_type, const int change)
	{
		this->set_troop_attack_modifier(troop_type, this->get_troop_attack_modifier(troop_type) + change);
	}

	int get_troop_defense(troop_type *troop_type) const;

	int get_troop_defense_modifier(troop_type *troop_type) const
	{
		auto find_iterator = this->troop_defense_modifiers.find(troop_type);
		if (find_iterator == this->troop_defense_modifiers.end()) {
			return 0;
		}

		return find_iterator->second;
	}

	void set_troop_defense_modifier(troop_type *troop_type, const int modifier)
	{
		if (modifier == this->get_troop_defense_modifier(troop_type)) {
			return;
		}

		if (modifier == 0) {
			this->troop_defense_modifiers.erase(troop_type);
		} else {
			this->troop_defense_modifiers[troop_type] = modifier;
		}

		emit troop_stats_changed();
	}

	void change_troop_defense_modifier(troop_type *troop_type, const int change)
	{
		this->set_troop_defense_modifier(troop_type, this->get_troop_defense_modifier(troop_type) + change);
	}

	bool is_selected() const
	{
		return this->selected;
	}

	void set_selected(const bool selected, const bool notify_engine_interface = true);

	Q_INVOKABLE QVariantList get_population_per_type_qvariant_list() const;
	Q_INVOKABLE QVariantList get_population_per_culture_qvariant_list() const;
	Q_INVOKABLE QVariantList get_population_per_religion_qvariant_list() const;
	Q_INVOKABLE void order_construction(const QVariant &building_variant);

private:
	building_slot *get_building_slot(building *building) const
	{
		auto find_iterator = this->building_slots.find(building);
		if (find_iterator == this->building_slots.end()) {
			return nullptr;
		}

		return find_iterator->second.get();
	}

signals:
	void name_changed();
	void titled_name_changed();
	void type_changed();
	void portrait_path_changed();
	void owner_changed();
	void population_units_changed();
	void population_changed();
	void population_capacity_changed();
	void population_growth_changed();
	void population_groups_changed();
	void building_slots_changed();
	void buildings_changed();
	void under_construction_building_changed();
	void construction_days_changed();
	void terrain_changed();
	void commodity_changed();
	void culture_changed();
	void religion_changed();
	void active_trade_routes_changed();
	void levies_changed();
	void troop_stats_changed();
	void selected_changed();

private:
	holding_slot *slot = nullptr;
	holding_type *type = nullptr;
	character *owner = nullptr; //the owner of the holding
	std::vector<qunique_ptr<population_unit>> population_units;
	int base_population_capacity = 0; //the base population capacity
	int population_capacity_modifier = 100; //the population capacity modifier
	int population_capacity = 0; //the population capacity
	int population = 0; //the size of this holding's total population
	int base_population_growth = 0; //the base population growth
	int population_growth = 0; //the population growth, in permyriad (per 10,000)
	std::map<building *, qunique_ptr<building_slot>> building_slots; //the building slots for each building
	building *under_construction_building = nullptr; //the building currently under construction
	int construction_days = 0; //the amount of days remaining to construct the building under construction
	metternich::commodity *commodity = nullptr; //the commodity produced by the holding (if any)
	metternich::culture *culture = nullptr; //the holding's culture
	metternich::religion *religion = nullptr; //the holding's religion
	std::set<holding_modifier *> modifiers; //modifiers applied to the holding
	std::set<employment *> employments;
	std::map<population_type *, int> population_per_type; //the population for each population type
	std::map<metternich::culture *, int> population_per_culture; //the population for each culture
	std::map<metternich::religion *, int> population_per_religion; //the population for each religion
	mutable std::shared_mutex population_groups_mutex;
	troop_type_map<int> levies; //levies per troop type
	troop_type_map<int> troop_attack_modifiers;
	troop_type_map<int> troop_defense_modifiers;
	bool selected = false;
};

}
