#pragma once

#include "economy/commodity_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "script/scripted_modifier_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("infrastructure/improvement.h")
Q_MOC_INCLUDE("infrastructure/settlement_type.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("population/population.h")

namespace metternich {

class building_class;
class building_type;
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
class scripted_site_modifier;
class settlement_building_slot;
class settlement_type;
class site;
class tile;

class site_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos CONSTANT)
	Q_PROPERTY(metternich::province* province READ get_province_unconst CONSTANT)
	Q_PROPERTY(metternich::country* owner READ get_owner_unconst NOTIFY owner_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(metternich::settlement_type* settlement_type READ get_settlement_type_unconst NOTIFY settlement_type_changed)
	Q_PROPERTY(metternich::improvement* improvement READ get_improvement_unconst NOTIFY improvement_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(int housing READ get_housing NOTIFY housing_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)

public:
	static constexpr int base_free_food_consumption = 1;
	static constexpr int base_settlement_score = 10;

	explicit site_game_data(const metternich::site *site);

	void do_turn();
	void do_consumption();

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

private:
	//for the Qt property (pointers there can't be const)
	province *get_province_unconst() const
	{
		return const_cast<province *>(this->get_province());
	}

public:
	bool is_provincial_capital() const;
	bool is_capital() const;
	bool can_be_capital() const;

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

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change);

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

	int get_housing() const
	{
		return this->housing;
	}

	void change_housing(const int change);

	int get_available_housing() const
	{
		return this->get_housing() - this->get_population_unit_count();
	}

	int get_free_food_consumption() const
	{
		return this->free_food_consumption;
	}

	const commodity_map<int> &get_base_commodity_outputs() const
	{
		return this->base_commodity_outputs;
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

	const centesimal_int &get_local_commodity_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->local_commodity_consumptions.find(commodity);

		if (find_iterator != this->local_commodity_consumptions.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	void change_local_commodity_consumption(const commodity *commodity, const centesimal_int &change);

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
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void improvement_changed();
	void settlement_type_changed();
	void scripted_modifiers_changed();
	void population_units_changed();
	void housing_changed();
	void commodity_outputs_changed();

private:
	const metternich::site *site = nullptr;
	const country *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::settlement_type *settlement_type = nullptr;
	std::vector<qunique_ptr<settlement_building_slot>> building_slots;
	building_slot_type_map<settlement_building_slot *> building_slot_map;
	scripted_site_modifier_map<int> scripted_modifiers;
	int score = 0;
	std::vector<qunique_ptr<population_unit>> population_units;
	qunique_ptr<metternich::population> population;
	int housing = 0;
	int free_food_consumption = 0;
	commodity_map<int> base_commodity_outputs;
	commodity_map<int> commodity_outputs;
	commodity_map<centesimal_int> local_commodity_consumptions;
	int output_modifier = 0;
	commodity_map<int> commodity_output_modifiers;
	std::vector<military_unit *> visiting_military_units; //military units currently visiting the site
};

}
