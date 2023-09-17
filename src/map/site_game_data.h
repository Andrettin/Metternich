#pragma once

#include "economy/commodity_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "util/qunique_ptr.h"

namespace metternich {

class country;
class culture;
class improvement;
class military_unit;
class phenotype;
class population;
class population_type;
class population_unit;
class province;
class religion;
class resource;
class settlement_building_slot;
class settlement_type;
class site;
class tile;

class site_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(metternich::province* province READ get_province_unconst NOTIFY tile_pos_changed)
	Q_PROPERTY(metternich::country* owner READ get_owner_unconst NOTIFY owner_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(metternich::settlement_type* settlement_type READ get_settlement_type_unconst NOTIFY settlement_type_changed)
	Q_PROPERTY(metternich::improvement* improvement READ get_improvement_unconst NOTIFY improvement_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)

public:
	explicit site_game_data(const metternich::site *site);

	void reset_non_map_data();

	void do_turn();

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos);
	tile *get_tile() const;

	bool is_on_map() const
	{
		return this->tile_pos != QPoint(-1, -1);
	}

	const province *get_province() const;

private:
	//for the Qt property (pointers there can't be const)
	province *get_province_unconst() const
	{
		return const_cast<province *>(this->get_province());
	}

public:
	bool is_provincial_capital() const;
	bool is_capital() const;

	const country *get_owner() const
	{
		return this->owner;
	}

private:
	//for the Qt property (pointers there can't be const)
	country *get_owner_unconst() const
	{
		return const_cast<country *>(this->get_owner());
	}

public:
	void set_owner(const country *owner);

	const culture *get_culture() const;

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

	const religion *get_religion() const;

	const metternich::settlement_type *get_settlement_type() const
	{
		return this->settlement_type;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::settlement_type *get_settlement_type_unconst() const
	{
		return const_cast<metternich::settlement_type *>(this->get_settlement_type());
	}

public:
	void set_settlement_type(const metternich::settlement_type *settlement_type);

	bool is_built() const;

	const resource *get_resource() const;

	const improvement *get_improvement() const;

private:
	//for the Qt property (pointers there can't be const)
	improvement *get_improvement_unconst() const
	{
		return const_cast<improvement *>(this->get_improvement());
	}

public:
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
	bool has_building(const building_type *building) const;
	bool has_building_or_better(const building_type *building) const;
	void clear_buildings();
	void check_building_conditions();
	void check_free_buildings();
	bool check_free_building(const building_type *building);

	void on_building_gained(const building_type *building, const int multiplier);
	void on_wonder_gained(const wonder *wonder, const int multiplier);

	const std::vector<qunique_ptr<population_unit>> &get_population_units() const
	{
		return this->population_units;
	}

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);
	qunique_ptr<population_unit> pop_population_unit(population_unit *population_unit);
	void clear_population_units();
	void create_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype);

	metternich::population *get_population() const
	{
		return this->population.get();
	}

	void change_base_commodity_output(const commodity *commodity, const int change);

	const commodity_map<int> &get_commodity_outputs() const
	{
		return this->commodity_outputs;
	}

	QVariantList get_commodity_outputs_qvariant_list() const;

	int get_commodity_output(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_outputs.find(commodity);
		if (find_iterator != this->commodity_outputs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_commodity_output(metternich::commodity *commodity)
	{
		const metternich::commodity *const_commodity = commodity;
		return this->get_commodity_output(const_commodity);
	}

	void set_commodity_output(const commodity *commodity, const int output);
	void calculate_commodity_outputs();

	const std::vector<military_unit *> &get_visiting_military_units() const
	{
		return this->visiting_military_units;
	}

	void add_visiting_military_unit(military_unit *military_unit)
	{
		this->visiting_military_units.push_back(military_unit);
	}

	void remove_visiting_military_unit(const military_unit *military_unit)
	{
		std::erase(this->visiting_military_units, military_unit);
	}

signals:
	void tile_pos_changed();
	void owner_changed();
	void culture_changed();
	void improvement_changed();
	void settlement_type_changed();
	void population_units_changed();
	void commodity_outputs_changed();

private:
	const metternich::site *site = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	const country *owner = nullptr;
	const metternich::settlement_type *settlement_type = nullptr;
	std::vector<qunique_ptr<settlement_building_slot>> building_slots;
	building_slot_type_map<settlement_building_slot *> building_slot_map;
	std::vector<qunique_ptr<population_unit>> population_units;
	qunique_ptr<metternich::population> population;
	commodity_map<int> base_commodity_outputs;
	commodity_map<int> commodity_outputs;
	std::vector<military_unit *> visiting_military_units; //military units currently visiting the site
};

}
