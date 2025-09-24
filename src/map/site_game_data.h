#pragma once

#include "database/data_entry_container.h"
#include "economy/commodity_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "script/scripted_modifier_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("infrastructure/dungeon.h")
Q_MOC_INCLUDE("infrastructure/holding_type.h")
Q_MOC_INCLUDE("infrastructure/improvement.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("population/population.h")

namespace archimedes {
	class gsml_data;
}

namespace metternich {

class army;
class building_class;
class building_type;
class culture;
class domain;
class dungeon;
class holding_type;
class improvement;
class party;
class pathway;
class phenotype;
class population;
class population_type;
class population_unit;
class portrait;
class province;
class religion;
class resource;
class scripted_site_modifier;
class settlement_building_slot;
class site;
class tile;
enum class improvement_slot;
enum class site_tier;

class site_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos CONSTANT)
	Q_PROPERTY(const metternich::province* province READ get_province CONSTANT)
	Q_PROPERTY(QString title_name READ get_title_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(const metternich::domain* owner READ get_owner NOTIFY owner_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(const metternich::holding_type* holding_type READ get_holding_type NOTIFY holding_type_changed)
	Q_PROPERTY(const metternich::dungeon* dungeon READ get_dungeon NOTIFY dungeon_changed)
	Q_PROPERTY(const metternich::improvement* improvement READ get_main_improvement NOTIFY improvements_changed)
	Q_PROPERTY(const metternich::improvement* resource_improvement READ get_resource_improvement NOTIFY improvements_changed)
	Q_PROPERTY(const metternich::portrait* portrait READ get_portrait NOTIFY portrait_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(int housing READ get_housing_int NOTIFY housing_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)
	Q_PROPERTY(QVariantList visiting_armies READ get_visiting_armies_qvariant_list NOTIFY visiting_armies_changed)

public:
	static constexpr int settlement_base_free_food_consumption = 1;

	explicit site_game_data(const metternich::site *site);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	void initialize_resource();
	void do_turn();

	const QPoint &get_tile_pos() const;
	tile *get_tile() const;

	bool is_on_map() const
	{
		return this->get_tile_pos() != QPoint(-1, -1);
	}

	bool is_coastal() const;
	bool is_near_water() const;
	bool has_route() const;
	bool has_pathway(const pathway *pathway) const;

	const province *get_province() const;

	bool is_provincial_capital() const;
	bool is_capital() const;
	bool can_be_capital() const;

	site_tier get_tier() const;

	const std::string &get_title_name() const;

	QString get_title_name_qstring() const
	{
		return QString::fromStdString(this->get_title_name());
	}

	const domain *get_owner() const
	{
		return this->owner;
	}

	void set_owner(const domain *owner);

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	void set_culture(const metternich::culture *culture);
	void on_population_main_culture_changed(const metternich::culture *culture);

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	void set_religion(const metternich::religion *religion);
	void on_population_main_religion_changed(const metternich::religion *religion);

	const metternich::holding_type *get_holding_type() const
	{
		return this->holding_type;
	}

	void set_holding_type(const metternich::holding_type *holding_type);
	void check_holding_type();
	std::vector<const metternich::holding_type *> get_best_holding_types(const std::vector<const metternich::holding_type *> &holding_types) const;

	Q_INVOKABLE bool is_built() const;

	const resource *get_resource() const;

	bool is_resource_discovered() const
	{
		return this->resource_discovered;
	}

	void set_resource_discovered(const bool discovered)
	{
		this->resource_discovered = discovered;
	}

	const metternich::dungeon *get_dungeon() const
	{
		return this->dungeon;
	}

	void set_dungeon(const metternich::dungeon *dungeon);
	bool can_have_dungeon(const metternich::dungeon *dungeon) const;

	const improvement *get_improvement(const improvement_slot slot) const
	{
		const auto find_iterator = this->improvements.find(slot);

		if (find_iterator != this->improvements.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	const improvement *get_main_improvement() const;
	const improvement *get_resource_improvement() const;
	bool has_improvement(const improvement *improvement) const;
	bool has_improvement_or_better(const improvement *improvement) const;
	void set_improvement(const improvement_slot slot, const improvement *improvement);

	const portrait *get_portrait() const;

	const std::vector<qunique_ptr<settlement_building_slot>> &get_building_slots() const
	{
		return this->building_slots;
	}

	QVariantList get_building_slots_qvariant_list() const;
	void initialize_building_slots();

	settlement_building_slot *get_building_slot(const building_slot_type *slot_type) const
	{
		const auto find_iterator = this->building_slot_map.find(slot_type);

		if (find_iterator != this->building_slot_map.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	const building_type *get_slot_building(const building_slot_type *slot_type) const;
	void set_slot_building(const building_slot_type *slot_type, const building_type *building);
	const building_type *get_building_class_type(const building_class *building_class) const;
	bool has_building(const building_type *building) const;
	bool has_building_or_better(const building_type *building) const;
	bool has_building_class(const building_class *building_class) const;
	bool has_building_class_or_better(const building_class *building_class) const;
	bool can_gain_building(const building_type *building) const;
	bool can_gain_building_class(const building_class *building_class) const;
	void add_building(const building_type *building);
	void clear_buildings();
	void check_building_conditions();
	void check_free_buildings();
	bool check_free_building(const building_type *building);
	bool check_free_improvement(const improvement *improvement);

	void on_settlement_built(const int multiplier);
	void on_building_gained(const building_type *building, const int multiplier);
	void on_wonder_gained(const wonder *wonder, const int multiplier);
	void on_improvement_gained(const improvement *improvement, const int multiplier);

	const scripted_site_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_site_modifier *modifier) const;
	void add_scripted_modifier(const scripted_site_modifier *modifier, const int duration);
	void remove_scripted_modifier(const scripted_site_modifier *modifier);
	void decrement_scripted_modifiers();

	Q_INVOKABLE bool can_have_population() const;
	bool can_have_population_type(const population_type *type) const;

	const std::vector<qunique_ptr<population_unit>> &get_population_units() const
	{
		return this->population_units;
	}

	int get_population_unit_count() const
	{
		return static_cast<int>(this->get_population_units().size());
	}

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);
	qunique_ptr<population_unit> pop_population_unit(population_unit *population_unit);
	void clear_population_units();
	void create_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype);

	metternich::population *get_population() const
	{
		return this->population.get();
	}

	void on_population_type_count_changed(const population_type *type, const int change);

	const population_class *get_default_population_class() const;
	const population_class *get_default_literate_population_class() const;

	const centesimal_int &get_housing() const
	{
		return this->housing;
	}

	int get_housing_int() const
	{
		return this->get_housing().to_int();
	}

	void change_housing(const centesimal_int &change);

	centesimal_int get_available_housing() const
	{
		return this->get_housing() - this->get_population_unit_count();
	}

	int get_free_food_consumption() const
	{
		return this->free_food_consumption;
	}

	void change_free_food_consumption(const int change)
	{
		this->free_food_consumption += change;
	}

	const commodity_map<centesimal_int> &get_base_commodity_outputs() const
	{
		return this->base_commodity_outputs;
	}

	void change_base_commodity_output(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_commodity_outputs() const
	{
		return this->commodity_outputs;
	}

	QVariantList get_commodity_outputs_qvariant_list() const;

	const centesimal_int &get_commodity_output(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_outputs.find(commodity);
		if (find_iterator != this->commodity_outputs.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_commodity_output(metternich::commodity *commodity)
	{
		const metternich::commodity *const_commodity = commodity;
		return this->get_commodity_output(const_commodity).to_int();
	}

	void set_commodity_output(const commodity *commodity, const centesimal_int &output);
	void calculate_commodity_outputs();

	const centesimal_int &get_output_modifier() const
	{
		return this->output_modifier;
	}

	void set_output_modifier(const centesimal_int &value)
	{
		if (value == this->get_output_modifier()) {
			return;
		}

		this->output_modifier = value;

		this->calculate_commodity_outputs();
	}

	void change_output_modifier(const centesimal_int &change)
	{
		this->set_output_modifier(this->get_output_modifier() + change);
	}

	int get_resource_output_modifier() const
	{
		return this->resource_output_modifier;
	}

	void set_resource_output_modifier(const int value)
	{
		if (value == this->get_resource_output_modifier()) {
			return;
		}

		this->resource_output_modifier = value;

		this->calculate_commodity_outputs();
	}

	void change_resource_output_modifier(const int value)
	{
		this->set_resource_output_modifier(this->get_resource_output_modifier() + value);
	}

	const commodity_map<centesimal_int> &get_commodity_output_modifiers() const
	{
		return this->commodity_output_modifiers;
	}

	const centesimal_int &get_commodity_output_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_output_modifiers.find(commodity);

		if (find_iterator != this->commodity_output_modifiers.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
	{
		if (value == this->get_commodity_output_modifier(commodity)) {
			return;
		}

		if (value == 0) {
			this->commodity_output_modifiers.erase(commodity);
		} else {
			this->commodity_output_modifiers[commodity] = value;
		}

		this->calculate_commodity_outputs();
	}

	void change_commodity_output_modifier(const commodity *commodity, const centesimal_int &change)
	{
		this->set_commodity_output_modifier(commodity, this->get_commodity_output_modifier(commodity) + change);
	}

	bool produces_commodity(const commodity *commodity) const
	{
		return this->get_commodity_outputs().contains(commodity);
	}

	bool can_be_visited() const;

	const std::vector<army *> &get_visiting_armies() const
	{
		return this->visiting_armies;
	}

	QVariantList get_visiting_armies_qvariant_list() const;

	void add_visiting_army(army *army)
	{
		this->visiting_armies.push_back(army);
		emit visiting_armies_changed();
	}

	void remove_visiting_army(army *army)
	{
		std::erase(this->visiting_armies, army);
		emit visiting_armies_changed();
	}

	void explore_dungeon(const std::shared_ptr<party> &party);
	std::vector<const dungeon_area *> get_potential_dungeon_areas() const;
	const data_entry_set<dungeon_area> &get_explored_dungeon_areas() const;
	void add_explored_dungeon_area(const dungeon_area *dungeon_area);

signals:
	void title_name_changed();
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void improvements_changed();
	void holding_type_changed();
	void dungeon_changed();
	void portrait_changed();
	void scripted_modifiers_changed();
	void population_units_changed();
	void housing_changed();
	void commodity_outputs_changed();
	void visiting_armies_changed();

private:
	const metternich::site *site = nullptr;
	const domain *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::holding_type *holding_type = nullptr;
	const metternich::dungeon *dungeon = nullptr;
	data_entry_set<dungeon_area> explored_dungeon_areas;
	std::map<improvement_slot, const improvement *> improvements;
	bool resource_discovered = false;
	std::vector<qunique_ptr<settlement_building_slot>> building_slots;
	building_slot_type_map<settlement_building_slot *> building_slot_map;
	scripted_site_modifier_map<int> scripted_modifiers;
	std::vector<qunique_ptr<population_unit>> population_units;
	qunique_ptr<metternich::population> population;
	centesimal_int housing;
	int free_food_consumption = 0;
	commodity_map<centesimal_int> base_commodity_outputs;
	commodity_map<centesimal_int> commodity_outputs;
	centesimal_int output_modifier;
	int resource_output_modifier = 0;
	commodity_map<centesimal_int> commodity_output_modifiers;
	std::vector<army *> visiting_armies; //armies visiting this site
};

}
