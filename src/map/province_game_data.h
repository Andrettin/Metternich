#pragma once

#include "country/culture_container.h"
#include "country/religion_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "infrastructure/building_type_container.h"
#include "map/terrain_type_container.h"
#include "script/scripted_modifier_container.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("country/culture.h")
Q_MOC_INCLUDE("country/religion.h")
Q_MOC_INCLUDE("population/population.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class army;
class civilian_unit;
class commodity;
class country;
class culture;
class employment_location;
class icon;
class improvement;
class military_unit;
class phenotype;
class population;
class population_unit;
class province;
class religion;
class scripted_province_modifier;
class site;
class tile;
class wonder;
enum class military_unit_category;

template <typename scope_type>
class modifier;

class province_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::country* owner READ get_owner NOTIFY owner_changed)
	Q_PROPERTY(const metternich::culture* culture READ get_culture NOTIFY culture_changed)
	Q_PROPERTY(const metternich::religion* religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(bool coastal READ is_coastal CONSTANT)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect CONSTANT)
	Q_PROPERTY(QPoint center_tile_pos READ get_center_tile_pos CONSTANT)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(QVariantList military_units READ get_military_units_qvariant_list NOTIFY military_units_changed)
	Q_PROPERTY(QVariantList military_unit_category_counts READ get_military_unit_category_counts_qvariant_list NOTIFY military_unit_category_counts_changed)
	Q_PROPERTY(QVariantList entering_armies READ get_entering_armies_qvariant_list NOTIFY entering_armies_changed)

public:
	explicit province_game_data(const metternich::province *province);
	province_game_data(const province_game_data &other) = delete;
	~province_game_data();

	void do_turn();
	void do_events();
	void do_ai_turn();

	bool is_on_map() const;

	const country *get_owner() const
	{
		return this->owner;
	}

	void set_owner(const country *country);

	bool is_capital() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	void set_culture(const metternich::culture *culture);
	void on_population_main_culture_changed(const metternich::culture *culture);

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	void set_religion(const metternich::religion *religion);
	void on_population_main_religion_changed(const metternich::religion *religion);

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

	bool is_coastal() const;
	bool is_near_water() const;

	const QRect &get_territory_rect() const;
	const QPoint &get_territory_rect_center() const;
	const std::vector<const metternich::province *> &get_neighbor_provinces() const;

	bool is_country_border_province() const;

	const QPoint &get_center_tile_pos() const;

	const std::vector<QPoint> &get_border_tiles() const;
	const std::vector<QPoint> &get_resource_tiles() const;
	const std::vector<const site *> &get_sites() const;
	const std::vector<const site *> &get_settlement_sites() const;

	int get_settlement_count() const
	{
		return this->settlement_count;
	}

	void change_settlement_count(const int change)
	{
		this->settlement_count += change;
	}

	const resource_map<int> &get_resource_counts() const;
	const terrain_type_map<int> &get_tile_terrain_counts() const;

	bool produces_commodity(const commodity *commodity) const;

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

	const std::vector<population_unit *> &get_population_units() const
	{
		return this->population_units;
	}

	int get_population_unit_count() const
	{
		return static_cast<int>(this->get_population_units().size());
	}

	void add_population_unit(population_unit *population_unit);
	void remove_population_unit(population_unit *population_unit);
	void clear_population_units();

	metternich::population *get_population() const
	{
		return this->population.get();
	}

	const std::vector<military_unit *> &get_military_units() const
	{
		return this->military_units;
	}

	QVariantList get_military_units_qvariant_list() const;

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
	Q_INVOKABLE QVariantList get_country_military_unit_category_counts(metternich::country *country) const;
	Q_INVOKABLE int get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::country *country) const;

	Q_INVOKABLE const icon *get_military_unit_icon() const;
	Q_INVOKABLE const icon *get_military_unit_category_icon(const metternich::military_unit_category category) const;
	Q_INVOKABLE QString get_military_unit_category_name(const metternich::military_unit_category category) const;
	Q_INVOKABLE const icon *get_country_military_unit_icon(metternich::country *country) const;

	const std::vector<army *> &get_entering_armies() const
	{
		return this->entering_armies;
	}

	QVariantList get_entering_armies_qvariant_list() const;

	void add_entering_army(army *army)
	{
		this->entering_armies.push_back(army);
		emit entering_armies_changed();
	}

	void remove_entering_army(army *army)
	{
		std::erase(this->entering_armies, army);
		emit entering_armies_changed();
	}

	void calculate_site_commodity_outputs();
	void calculate_site_commodity_output(const commodity *commodity);

	const centesimal_int &get_local_commodity_output(const commodity *commodity) const
	{
		const auto find_iterator = this->local_commodity_outputs.find(commodity);
		if (find_iterator != this->local_commodity_outputs.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void change_local_commodity_output(const commodity *commodity, const centesimal_int &change);

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

		this->calculate_site_commodity_outputs();
	}

	void change_output_modifier(const centesimal_int &change)
	{
		this->set_output_modifier(this->get_output_modifier() + change);
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

		this->calculate_site_commodity_output(commodity);
	}

	void change_commodity_output_modifier(const commodity *commodity, const centesimal_int &change)
	{
		this->set_commodity_output_modifier(commodity, this->get_commodity_output_modifier(commodity) + change);
	}

	const resource_map<commodity_map<int>> &get_improved_resource_commodity_bonuses() const
	{
		return this->improved_resource_commodity_bonuses;
	}

	const commodity_map<int> &get_improved_resource_commodity_bonuses(const resource *resource) const
	{
		static const commodity_map<int> empty_map;

		const auto find_iterator = this->improved_resource_commodity_bonuses.find(resource);

		if (find_iterator != this->improved_resource_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		return empty_map;
	}

	int get_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity) const
	{
		const auto find_iterator = this->improved_resource_commodity_bonuses.find(resource);

		if (find_iterator != this->improved_resource_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void change_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity, const int change);

	const commodity_map<std::map<int, int>> &get_commodity_bonuses_for_tile_thresholds() const
	{
		return this->commodity_bonuses_for_tile_thresholds;
	}

	const std::map<int, int> &get_commodity_bonus_for_tile_threshold_map(const commodity *commodity) const
	{
		static std::map<int, int> empty_map;

		const auto find_iterator = this->commodity_bonuses_for_tile_thresholds.find(commodity);

		if (find_iterator != this->commodity_bonuses_for_tile_thresholds.end()) {
			return find_iterator->second;
		}

		return empty_map;
	}

	int get_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold) const
	{
		const std::map<int, int> &threshold_map = this->get_commodity_bonus_for_tile_threshold_map(commodity);

		const auto find_iterator = threshold_map.find(threshold);
		if (find_iterator != threshold_map.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value);

	void change_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
	{
		this->set_commodity_bonus_for_tile_threshold(commodity, threshold, this->get_commodity_bonus_for_tile_threshold(commodity, threshold) + value);
	}

	bool can_produce_commodity(const commodity *commodity) const;

	std::vector<employment_location *> get_employment_locations() const;
	void check_employment();
	void check_available_employment(const std::vector<employment_location *> &employment_locations, std::vector<population_unit *> &unemployed_population_units);

	province_game_data &operator =(const province_game_data &other) = delete;

signals:
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void scripted_modifiers_changed();
	void population_units_changed();
	void military_units_changed();
	void military_unit_category_counts_changed();
	void entering_armies_changed();

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	int settlement_count = 0; //only includes built settlements
	scripted_province_modifier_map<int> scripted_modifiers;
	std::vector<population_unit *> population_units;
	qunique_ptr<metternich::population> population;
	std::vector<military_unit *> military_units;
	std::map<military_unit_category, int> military_unit_category_counts;
	std::vector<army *> entering_armies; //armies entering this province
	commodity_map<centesimal_int> local_commodity_outputs;
	centesimal_int output_modifier;
	commodity_map<centesimal_int> commodity_output_modifiers;
	resource_map<commodity_map<int>> improved_resource_commodity_bonuses;
	commodity_map<std::map<int, int>> commodity_bonuses_for_tile_thresholds;
};

}
