#pragma once

#include "country/culture_container.h"
#include "country/ideology_container.h"
#include "country/religion_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "map/terrain_type_container.h"
#include "population/phenotype_container.h"
#include "population/population_type_container.h"
#include "script/scripted_modifier_container.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

namespace metternich {

class building_slot;
class building_type;
class civilian_unit;
class commodity;
class country;
class culture;
class icon;
class improvement;
class military_unit;
class phenotype;
class population_type;
class population_unit;
class province;
class religion;
class scripted_province_modifier;
class site;
class tile;
enum class military_unit_category;

template <typename scope_type>
class modifier;

class province_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner READ get_owner_unconst NOTIFY owner_changed)
	Q_PROPERTY(metternich::culture* culture READ get_culture_unconst NOTIFY culture_changed)
	Q_PROPERTY(metternich::religion* religion READ get_religion_unconst NOTIFY religion_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(bool coastal READ is_coastal CONSTANT)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY territory_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(QVariantList population_type_counts READ get_population_type_counts_qvariant_list NOTIFY population_type_counts_changed)
	Q_PROPERTY(QVariantList population_culture_counts READ get_population_culture_counts_qvariant_list NOTIFY population_culture_counts_changed)
	Q_PROPERTY(QVariantList population_religion_counts READ get_population_religion_counts_qvariant_list NOTIFY population_religion_counts_changed)
	Q_PROPERTY(QVariantList population_phenotype_counts READ get_population_phenotype_counts_qvariant_list NOTIFY population_phenotype_counts_changed)
	Q_PROPERTY(QVariantList population_ideology_counts READ get_population_ideology_counts_qvariant_list NOTIFY population_ideology_counts_changed)
	Q_PROPERTY(int population READ get_population NOTIFY population_changed)
	Q_PROPERTY(int consciousness READ get_consciousness_int NOTIFY consciousness_changed)
	Q_PROPERTY(int militancy READ get_militancy_int NOTIFY militancy_changed)
	Q_PROPERTY(QVariantList military_unit_category_counts READ get_military_unit_category_counts_qvariant_list NOTIFY military_unit_category_counts_changed)

public:
	static constexpr int base_free_food_consumption = 1;

	explicit province_game_data(const metternich::province *province);
	province_game_data(const province_game_data &other) = delete;
	~province_game_data();

	void reset_non_map_data();

	void on_map_created();

	void do_turn();
	void do_production();
	void do_cultural_change();
	void do_events();
	void do_ai_turn();

	bool is_on_map() const
	{
		return !this->get_tiles().empty();
	}

	const country *get_owner() const
	{
		return this->owner;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_owner_unconst() const
	{
		return const_cast<metternich::country *>(this->get_owner());
	}

public:
	void set_owner(const country *country);

	bool is_capital() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::culture *get_culture_unconst() const
	{
		return const_cast<metternich::culture *>(this->get_culture());
	}

public:
	void set_culture(const metternich::culture *culture);
	void calculate_culture();

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::religion *get_religion_unconst() const
	{
		return const_cast<metternich::religion *>(this->get_religion());
	}

public:
	void set_religion(const metternich::religion *religion);
	void calculate_religion();

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	const QPoint &get_territory_rect_center() const
	{
		return this->territory_rect_center;
	}

	void calculate_territory_rect_center();

	const std::vector<const metternich::province *> &get_neighbor_provinces() const
	{
		return this->neighbor_provinces;
	}

	void add_neighbor_province(const metternich::province *province);
	bool is_country_border_province() const;

	const std::vector<QPoint> &get_tiles() const
	{
		return this->tiles;
	}

	void add_tile(const QPoint &tile_pos);

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	void add_border_tile(const QPoint &tile_pos);

	const std::vector<QPoint> &get_resource_tiles() const
	{
		return this->resource_tiles;
	}

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

	const terrain_type_map<int> &get_tile_terrain_counts() const
	{
		return this->tile_terrain_counts;
	}

	commodity_map<centesimal_int> get_commodity_outputs() const;
	bool produces_commodity(const commodity *commodity) const;

	void on_improvement_gained(const improvement *improvement, const int multiplier);
	void setup_resource_improvements();

	QVariantList get_building_slots_qvariant_list() const;
	void initialize_building_slots();
	void add_capital_building_slots();
	void remove_capital_building_slots();
	const building_type *get_slot_building(const building_slot_type *slot_type) const;
	void set_slot_building(const building_slot_type *slot_type, const building_type *building);
	void clear_buildings();

	void add_capitol();
	void remove_capitol();

	void on_building_gained(const building_type *building, const int multiplier);

	const scripted_province_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_province_modifier *modifier) const;
	void add_scripted_modifier(const scripted_province_modifier *modifier, const int duration);
	void remove_scripted_modifier(const scripted_province_modifier *modifier);
	void decrement_scripted_modifiers();

	void apply_modifier(const modifier<const metternich::province> *modifier, const int multiplier = 1);

	void remove_modifier(const modifier<const metternich::province> *modifier)
	{
		this->apply_modifier(modifier, -1);
	}

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);
	qunique_ptr<population_unit> pop_population_unit(population_unit *population_unit);
	void create_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype);
	void clear_population_units();

	const std::vector<qunique_ptr<population_unit>> &get_population_units() const
	{
		return this->population_units;
	}

	int get_population_unit_count() const
	{
		return static_cast<int>(this->population_units.size());
	}

	const population_type_map<int> &get_population_type_counts() const
	{
		return this->population_type_counts;
	}

	QVariantList get_population_type_counts_qvariant_list() const;
	void change_population_type_count(const population_type *type, const int change);

	const culture_map<int> &get_population_culture_counts() const
	{
		return this->population_culture_counts;
	}

	QVariantList get_population_culture_counts_qvariant_list() const;
	void change_population_culture_count(const metternich::culture *culture, const int change);

	const religion_map<int> &get_population_religion_counts() const
	{
		return this->population_religion_counts;
	}

	QVariantList get_population_religion_counts_qvariant_list() const;
	void change_population_religion_count(const metternich::religion *religion, const int change);

	const phenotype_map<int> &get_population_phenotype_counts() const
	{
		return this->population_phenotype_counts;
	}

	QVariantList get_population_phenotype_counts_qvariant_list() const;
	void change_population_phenotype_count(const phenotype *phenotype, const int change);

	const ideology_map<int> &get_population_ideology_counts() const
	{
		return this->population_ideology_counts;
	}

	QVariantList get_population_ideology_counts_qvariant_list() const;
	void change_population_ideology_count(const ideology *ideology, const int change);

	int get_population() const
	{
		return this->population;
	}

	void change_population(const int change);

	void grow_population();
	void decrease_population();
	population_unit *choose_starvation_population_unit();

	Q_INVOKABLE QObject *get_population_type_small_icon(metternich::population_type *type) const;

	void assign_workers();
	void reassign_workers();
	void assign_worker(population_unit *population_unit);
	bool try_assign_worker_to_tile(population_unit *population_unit, tile *tile);
	void assign_worker_to_tile(population_unit *population_unit, tile *tile);
	bool try_assign_worker_to_building(population_unit *population_unit, building_slot *building_slot);
	void assign_worker_to_building(population_unit *population_unit, building_slot *building_slot);

	void unassign_worker(population_unit *population_unit);

	int get_food_consumption() const
	{
		return this->get_population_unit_count() + static_cast<int>(this->home_civilian_units.size()) + static_cast<int>(this->home_military_units.size());
	}

	int get_free_food_consumption() const
	{
		return this->free_food_consumption;
	}

	bool can_tile_employ_worker(const population_unit *population_unit, const tile *tile) const;
	bool can_building_employ_worker(const population_unit *population_unit, const building_slot *building_slot) const;
	bool has_employment_for_worker(const population_unit *population_unit) const;

	centesimal_int get_consciousness() const;

	int get_consciousness_int() const
	{
		return this->get_consciousness().to_int();
	}

	const centesimal_int &get_total_consciousness() const
	{
		return this->total_consciousness;
	}

	void set_total_consciousness(const centesimal_int &consciousness);

	void change_total_consciousness(const centesimal_int &change)
	{
		this->set_total_consciousness(this->get_total_consciousness() + change);
	}

	centesimal_int get_militancy() const;

	int get_militancy_int() const
	{
		return this->get_militancy().to_int();
	}

	const centesimal_int &get_total_militancy() const
	{
		return this->total_militancy;
	}

	void set_total_militancy(const centesimal_int &militancy);

	void change_total_militancy(const centesimal_int &change)
	{
		this->set_total_militancy(this->get_total_militancy() + change);
	}

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change);

	void add_home_civilian_unit(civilian_unit *civilian_unit)
	{
		this->home_civilian_units.push_back(civilian_unit);
	}

	void remove_home_civilian_unit(civilian_unit *civilian_unit)
	{
		std::erase(this->home_civilian_units, civilian_unit);
	}

	void add_home_military_unit(military_unit *military_unit)
	{
		this->home_military_units.push_back(military_unit);
	}

	void remove_home_military_unit(military_unit *military_unit)
	{
		std::erase(this->home_military_units, military_unit);
	}

	const std::vector<military_unit *> &get_military_units() const
	{
		return this->military_units;
	}

	void add_military_unit(military_unit *military_unit);
	void remove_military_unit(military_unit *military_unit);
	void clear_military_units();

	QVariantList get_military_unit_category_counts_qvariant_list() const;

	Q_INVOKABLE int get_military_unit_category_count(const metternich::military_unit_category category) const
	{
		const auto find_iterator = this->military_unit_category_counts.find(category);

		if (find_iterator != this->military_unit_category_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_military_unit_category_count(const military_unit_category category, const int change);

	bool has_country_military_unit(const country *country) const;
	Q_INVOKABLE int get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::country *country) const;

	Q_INVOKABLE QObject *get_military_unit_category_icon(const metternich::military_unit_category category) const;
	Q_INVOKABLE QString get_military_unit_category_name(const metternich::military_unit_category category) const;

	int get_production_modifier() const
	{
		return this->production_modifier;
	}

	void set_production_modifier(const int value)
	{
		if (value == this->get_production_modifier()) {
			return;
		}

		this->production_modifier = value;

		this->calculate_base_commodity_outputs();
	}

	void change_production_modifier(const int value)
	{
		this->set_production_modifier(this->get_production_modifier() + value);
	}

	int get_commodity_production_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_production_modifiers.find(commodity);

		if (find_iterator != this->commodity_production_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_production_modifier(const commodity *commodity, const int value)
	{
		if (value == this->get_commodity_production_modifier(commodity)) {
			return;
		}

		if (value == 0) {
			this->commodity_production_modifiers.erase(commodity);
		} else {
			this->commodity_production_modifiers[commodity] = value;
		}

		this->calculate_base_commodity_outputs();
	}

	void change_commodity_production_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_production_modifier(commodity, this->get_commodity_production_modifier(commodity) + value);
	}

	void calculate_base_commodity_outputs();

	void apply_country_modifier(const country *country, const int multiplier);

	province_game_data &operator =(const province_game_data &other) = delete;

signals:
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void territory_changed();
	void scripted_modifiers_changed();
	void population_units_changed();
	void population_type_counts_changed();
	void population_culture_counts_changed();
	void population_religion_counts_changed();
	void population_phenotype_counts_changed();
	void population_ideology_counts_changed();
	void population_changed();
	void consciousness_changed();
	void militancy_changed();
	void military_units_changed();
	void military_unit_category_counts_changed();

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	bool coastal = false;
	QRect territory_rect;
	QPoint territory_rect_center = QPoint(-1, -1);
	std::vector<const metternich::province *> neighbor_provinces;
	std::vector<QPoint> tiles;
	std::vector<QPoint> border_tiles;
	std::vector<QPoint> resource_tiles;
	std::vector<const site *> sites;
	resource_map<int> resource_counts;
	terrain_type_map<int> tile_terrain_counts;
	std::vector<qunique_ptr<building_slot>> building_slots;
	building_slot_type_map<building_slot *> building_slot_map;
	scripted_province_modifier_map<int> scripted_modifiers;
	std::vector<qunique_ptr<population_unit>> population_units;
	population_type_map<int> population_type_counts;
	culture_map<int> population_culture_counts;
	religion_map<int> population_religion_counts;
	phenotype_map<int> population_phenotype_counts;
	ideology_map<int> population_ideology_counts;
	int population = 0;
	int free_food_consumption = 0;
	centesimal_int total_consciousness;
	centesimal_int total_militancy;
	int score = 0;
	std::vector<civilian_unit *> home_civilian_units;
	std::vector<military_unit *> home_military_units;
	std::vector<military_unit *> military_units;
	std::map<military_unit_category, int> military_unit_category_counts;
	int production_modifier = 0;
	commodity_map<int> commodity_production_modifiers;
};

}
