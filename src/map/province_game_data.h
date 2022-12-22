#pragma once

#include "country/culture_container.h"
#include "economy/resource_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "map/terrain_type_container.h"
#include "population/phenotype_container.h"
#include "population/population_type_container.h"
#include "util/qunique_ptr.h"

namespace metternich {

class building_slot;
class building_type;
class civilian_unit;
class country;
class culture;
class icon;
class improvement;
class phenotype;
class population_type;
class population_unit;
class province;
class tile;

class province_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner READ get_owner_unconst NOTIFY owner_changed)
	Q_PROPERTY(metternich::culture* culture READ get_culture_unconst NOTIFY culture_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY territory_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(QVariantList population_type_counts READ get_population_type_counts_qvariant_list NOTIFY population_type_counts_changed)
	Q_PROPERTY(QVariantList population_culture_counts READ get_population_culture_counts_qvariant_list NOTIFY population_culture_counts_changed)
	Q_PROPERTY(QVariantList population_phenotype_counts READ get_population_phenotype_counts_qvariant_list NOTIFY population_phenotype_counts_changed)
	Q_PROPERTY(int population READ get_population NOTIFY population_changed)

public:
	static constexpr int base_free_food_consumption = 1;

	explicit province_game_data(const metternich::province *province);
	province_game_data(const province_game_data &other) = delete;
	~province_game_data();

	void reset_non_map_data();

	void do_turn();
	void do_production();
	void do_cultural_change();

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

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
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

	const std::vector<const metternich::province *> &get_border_provinces() const
	{
		return this->border_provinces;
	}

	void add_border_province(const metternich::province *province)
	{
		this->border_provinces.push_back(province);
	}

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

	void on_improvement_gained(const improvement *improvement, const int multiplier);

	QVariantList get_building_slots_qvariant_list() const;
	const building_type *get_slot_building(const building_slot_type *slot_type) const;
	void set_slot_building(const building_slot_type *slot_type, const building_type *building);
	void clear_buildings();

	void add_capitol();
	void remove_capitol();

	void on_building_gained(const building_type *building, const int multiplier);

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);
	qunique_ptr<population_unit> pop_population_unit(population_unit *population_unit);
	void create_population_unit(const population_type *type, const metternich::culture *culture, const phenotype *phenotype);
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

	const phenotype_map<int> &get_population_phenotype_counts() const
	{
		return this->population_phenotype_counts;
	}

	QVariantList get_population_phenotype_counts_qvariant_list() const;
	void change_population_phenotype_count(const phenotype *phenotype, const int change);

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
		return this->get_population_unit_count() + static_cast<int>(this->civilian_units.size());
	}

	int get_free_food_consumption() const
	{
		return this->free_food_consumption;
	}

	bool can_tile_employ_worker(const population_unit *population_unit, const tile *tile) const;
	bool can_building_employ_worker(const population_unit *population_unit, const building_slot *building_slot) const;
	bool has_employment_for_worker(const population_unit *population_unit) const;

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change);

	void add_civilian_unit(civilian_unit *civilian_unit)
	{
		this->civilian_units.push_back(civilian_unit);
	}

	void remove_civilian_unit(civilian_unit *civilian_unit)
	{
		std::erase(this->civilian_units, civilian_unit);
	}

	province_game_data &operator =(const province_game_data &other) = delete;

signals:
	void owner_changed();
	void culture_changed();
	void territory_changed();
	void population_units_changed();
	void population_type_counts_changed();
	void population_culture_counts_changed();
	void population_phenotype_counts_changed();
	void population_changed();

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
	const metternich::culture *culture = nullptr;
	QRect territory_rect;
	QPoint territory_rect_center = QPoint(-1, -1);
	std::vector<const metternich::province *> border_provinces;
	std::vector<QPoint> tiles;
	std::vector<QPoint> border_tiles;
	std::vector<QPoint> resource_tiles;
	resource_map<int> resource_counts;
	terrain_type_map<int> tile_terrain_counts;
	std::vector<qunique_ptr<building_slot>> building_slots;
	building_slot_type_map<building_slot *> building_slot_map;
	std::vector<qunique_ptr<population_unit>> population_units;
	population_type_map<int> population_type_counts;
	culture_map<int> population_culture_counts;
	phenotype_map<int> population_phenotype_counts;
	int population = 0;
	int free_food_consumption = 0;
	int score = 0;
	std::vector<civilian_unit *> civilian_units;
};

}
