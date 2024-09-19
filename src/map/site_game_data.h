#pragma once

#include "economy/commodity_container.h"
#include "economy/employment_location.h"
#include "infrastructure/building_slot_type_container.h"
#include "script/scripted_modifier_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("infrastructure/improvement.h")
Q_MOC_INCLUDE("infrastructure/settlement_type.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("population/population.h")

namespace metternich {

class army;
class building_class;
class building_type;
class country;
class culture;
class improvement;
class phenotype;
class population;
class population_type;
class population_unit;
class province;
class religion;
class resource;
class scripted_site_modifier;
class settlement_building_slot;
class settlement_type;
class site;
class tile;
enum class improvement_slot;

class site_game_data final : public QObject, public employment_location
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos CONSTANT)
	Q_PROPERTY(const metternich::province* province READ get_province CONSTANT)
	Q_PROPERTY(const metternich::country* owner READ get_owner NOTIFY owner_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(const metternich::settlement_type* settlement_type READ get_settlement_type NOTIFY settlement_type_changed)
	Q_PROPERTY(const metternich::improvement* improvement READ get_main_improvement NOTIFY improvements_changed)
	Q_PROPERTY(const metternich::improvement* resource_improvement READ get_resource_improvement NOTIFY improvements_changed)
	Q_PROPERTY(const metternich::improvement* depot_improvement READ get_depot_improvement NOTIFY improvements_changed)
	Q_PROPERTY(const metternich::improvement* port_improvement READ get_port_improvement NOTIFY improvements_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(int health READ get_health_int NOTIFY health_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)
	Q_PROPERTY(int transport_level READ get_best_transport_level NOTIFY transport_level_changed)
	Q_PROPERTY(QVariantList visiting_armies READ get_visiting_armies_qvariant_list NOTIFY visiting_armies_changed)

public:
	static constexpr int base_free_food_consumption = 1;

	explicit site_game_data(const metternich::site *site);

	void do_turn();
	void do_everyday_consumption();
	void do_luxury_consumption();

	const QPoint &get_tile_pos() const;
	tile *get_tile() const;

	bool is_on_map() const
	{
		return this->get_tile_pos() != QPoint(-1, -1);
	}

	bool is_coastal() const;
	bool is_near_water() const;
	bool has_route() const;

	const province *get_province() const;

	bool is_provincial_capital() const;
	bool is_capital() const;
	bool can_be_capital() const;

	const country *get_owner() const
	{
		return this->owner;
	}

	void set_owner(const country *owner);

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

	const metternich::settlement_type *get_settlement_type() const
	{
		return this->settlement_type;
	}

	void set_settlement_type(const metternich::settlement_type *settlement_type);
	void check_settlement_type();

	bool is_built() const;

	const resource *get_resource() const;

	bool is_resource_discovered() const
	{
		return this->resource_discovered;
	}

	void set_resource_discovered(const bool discovered)
	{
		this->resource_discovered = discovered;
	}

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
	const improvement *get_depot_improvement() const;
	const improvement *get_port_improvement() const;
	bool has_improvement(const improvement *improvement) const;
	void set_improvement(const improvement_slot slot, const improvement *improvement);

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

	virtual const site *get_employment_site() const override;
	virtual const profession *get_employment_profession() const override;

	virtual bool is_resource_employment() const override
	{
		return true;
	}

	virtual int get_employment_output_multiplier() const override;

	const centesimal_int &get_health() const
	{
		return this->health;
	}

	int get_health_int() const
	{
		return this->get_health().to_int();
	}

	void change_health(const centesimal_int &change);

	centesimal_int get_available_health() const
	{
		return this->get_health() - this->get_population_unit_count();
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

	const centesimal_int &get_local_everyday_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->local_everyday_consumption.find(commodity);

		if (find_iterator != this->local_everyday_consumption.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	void change_local_everyday_consumption(const commodity *commodity, const centesimal_int &change);

	const centesimal_int &get_local_luxury_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->local_luxury_consumption.find(commodity);

		if (find_iterator != this->local_luxury_consumption.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	void change_local_luxury_consumption(const commodity *commodity, const centesimal_int &change);

	int get_output_modifier() const
	{
		return this->output_modifier;
	}

	void set_output_modifier(const int value)
	{
		if (value == this->get_output_modifier()) {
			return;
		}

		this->output_modifier = value;

		this->calculate_commodity_outputs();
	}

	void change_output_modifier(const int value)
	{
		this->set_output_modifier(this->get_output_modifier() + value);
	}

	const commodity_map<int> &get_commodity_output_modifiers() const
	{
		return this->commodity_output_modifiers;
	}

	int get_commodity_output_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_output_modifiers.find(commodity);

		if (find_iterator != this->commodity_output_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_output_modifier(const commodity *commodity, const int value)
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

	void change_commodity_output_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_output_modifier(commodity, this->get_commodity_output_modifier(commodity) + value);
	}

	bool produces_commodity(const commodity *commodity) const
	{
		return this->get_commodity_outputs().contains(commodity);
	}

	int get_depot_level() const
	{
		return this->depot_level;
	}

	void set_depot_level(const int level);

	void change_depot_level(const int change)
	{
		this->set_depot_level(this->get_depot_level() + change);
	}

	int get_port_level() const
	{
		return this->port_level;
	}

	void set_port_level(const int level);

	void change_port_level(const int change)
	{
		this->set_port_level(this->get_port_level() + change);
	}

	int get_transport_level() const
	{
		return this->transport_level;
	}

	void set_transport_level(const int level);

	int get_sea_transport_level() const
	{
		return this->sea_transport_level;
	}

	void set_sea_transport_level(const int level);

	int get_best_transport_level() const
	{
		return std::max(this->get_transport_level(), this->get_sea_transport_level());
	}

	centesimal_int get_transportable_commodity_output(const commodity *commodity) const;

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

signals:
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void improvements_changed();
	void settlement_type_changed();
	void scripted_modifiers_changed();
	void population_units_changed();
	void health_changed();
	void commodity_outputs_changed();
	void transport_level_changed();
	void visiting_armies_changed();

private:
	const metternich::site *site = nullptr;
	const country *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::settlement_type *settlement_type = nullptr;
	std::map<improvement_slot, const improvement *> improvements;
	bool resource_discovered = false;
	std::vector<qunique_ptr<settlement_building_slot>> building_slots;
	building_slot_type_map<settlement_building_slot *> building_slot_map;
	scripted_site_modifier_map<int> scripted_modifiers;
	std::vector<qunique_ptr<population_unit>> population_units;
	qunique_ptr<metternich::population> population;
	centesimal_int health;
	int free_food_consumption = 0;
	commodity_map<centesimal_int> base_commodity_outputs;
	commodity_map<centesimal_int> commodity_outputs;
	commodity_map<centesimal_int> local_everyday_consumption;
	commodity_map<centesimal_int> local_luxury_consumption;
	int output_modifier = 0;
	commodity_map<int> commodity_output_modifiers;
	int depot_level = 0;
	int port_level = 0;
	int transport_level = 0;
	int sea_transport_level = 0;
	std::vector<army *> visiting_armies; //armies visiting this site
};

}
