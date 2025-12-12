#pragma once

#include "domain/culture_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "infrastructure/building_type_container.h"
#include "map/terrain_type_container.h"
#include "religion/religion_container.h"
#include "script/scripted_modifier_container.h"
#include "technology/technology_container.h"
#include "unit/military_unit_type_container.h"
#include "util/centesimal_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("domain/culture.h")
Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("population/population.h")
Q_MOC_INCLUDE("religion/religion.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class army;
class character;
class civilian_unit;
class commodity;
class culture;
class domain;
class icon;
class improvement;
class military_unit;
class phenotype;
class population;
class population_unit;
class province;
class province_attribute;
class religion;
class scripted_province_modifier;
class site;
class skill;
class tile;
class wonder;
enum class military_unit_category;
enum class province_map_mode;

template <typename scope_type>
class modifier;

class province_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::domain* owner READ get_owner NOTIFY owner_changed)
	Q_PROPERTY(const metternich::culture* culture READ get_culture NOTIFY culture_changed)
	Q_PROPERTY(const metternich::religion* religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(int level READ get_level NOTIFY level_changed)
	Q_PROPERTY(int max_level READ get_max_level CONSTANT)
	Q_PROPERTY(QRect map_image_rect READ get_map_image_rect NOTIFY map_image_changed)
	Q_PROPERTY(QRect text_rect READ get_text_rect NOTIFY map_image_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(bool coastal READ is_coastal CONSTANT)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect CONSTANT)
	Q_PROPERTY(const metternich::site* provincial_capital READ get_provincial_capital NOTIFY provincial_capital_changed)
	Q_PROPERTY(QPoint center_tile_pos READ get_center_tile_pos CONSTANT)
	Q_PROPERTY(QVariantList attribute_values READ get_attribute_values_qvariant_list NOTIFY attribute_values_changed)
	Q_PROPERTY(QVariantList visible_sites READ get_visible_sites_qvariant_list NOTIFY visible_sites_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(QVariantList military_units READ get_military_units_qvariant_list NOTIFY military_units_changed)
	Q_PROPERTY(QVariantList military_unit_category_counts READ get_military_unit_category_counts_qvariant_list NOTIFY military_unit_category_counts_changed)
	Q_PROPERTY(QVariantList entering_armies READ get_entering_armies_qvariant_list NOTIFY entering_armies_changed)
	Q_PROPERTY(QVariantList recruitable_military_unit_categories READ get_recruitable_military_unit_categories_qvariant_list NOTIFY owner_changed)
	Q_PROPERTY(QVariantList military_unit_recruitment_counts READ get_military_unit_recruitment_counts_qvariant_list NOTIFY military_unit_recruitment_counts_changed)

public:
	explicit province_game_data(const metternich::province *province);
	province_game_data(const province_game_data &other) = delete;
	~province_game_data();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	void do_turn();
	void do_events();
	void do_ai_turn();
	int collect_taxes();
	void do_military_unit_recruitment();

	bool is_on_map() const;

	const domain *get_owner() const
	{
		return this->owner;
	}

	void set_owner(const domain *domain);

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

	int get_level() const
	{
		return this->level;
	}

	void set_level(const int level);
	void change_level(const int change);
	int get_max_level() const;

	bool is_coastal() const;
	bool is_near_water() const;

	const QRect &get_territory_rect() const;
	const QPoint &get_territory_rect_center() const;
	const std::vector<const metternich::province *> &get_neighbor_provinces() const;

	bool is_country_border_province() const;

	const site *get_provincial_capital() const;
	void set_provincial_capital(const site *site);
	void choose_provincial_capital();
	const site *get_best_provincial_capital_slot() const;
	const QPoint &get_center_tile_pos() const;

	const std::vector<QPoint> &get_border_tiles() const;
	const std::vector<QPoint> &get_resource_tiles() const;
	const std::vector<const site *> &get_sites() const;
	const std::vector<const site *> &get_settlement_sites() const;

	const QColor &get_map_color() const;

	const QImage &get_map_image() const
	{
		return this->map_image;
	}

	QImage prepare_map_image() const;

	[[nodiscard]]
	QCoro::Task<QImage> finalize_map_image(QImage &&image) const;

	[[nodiscard]]
	QCoro::Task<void> create_map_image();

	const QImage &get_selected_map_image() const
	{
		return this->selected_map_image;
	}

	const QImage &get_map_mode_image(const province_map_mode mode) const;

	[[nodiscard]]
	QCoro::Task<void> create_map_mode_image(const province_map_mode mode);

	const QRect &get_map_image_rect() const
	{
		return this->map_image_rect;
	}

	const QRect &get_text_rect() const
	{
		return this->text_rect;
	}

	void calculate_text_rect();

	const data_entry_map<province_attribute, int> &get_attribute_values() const
	{
		return this->attribute_values;
	}

	QVariantList get_attribute_values_qvariant_list() const;

	int get_attribute_value(const province_attribute *attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_attribute_value(const province_attribute *attribute, const int change);
	bool do_attribute_check(const province_attribute *attribute, const int roll_modifier) const;
	int get_attribute_check_chance(const province_attribute *attribute, const int roll_modifier) const;

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

	std::vector<const site *> get_visible_sites() const;
	QVariantList get_visible_sites_qvariant_list() const;

	bool produces_commodity(const commodity *commodity) const;

	const technology_set &get_technologies() const
	{
		return this->technologies;
	}

	QVariantList get_technologies_qvariant_list() const;

	bool has_technology(const metternich::technology *technology) const
	{
		return this->get_technologies().contains(technology);
	}

	void add_technology(const technology *technology);
	void add_technology_with_prerequisites(const technology *technology);
	void remove_technology(const technology *technology);

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
	std::vector<military_unit *> get_country_military_units(const domain *domain) const;
	Q_INVOKABLE QVariantList get_country_military_units_qvariant_list(const metternich::domain *domain) const;

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

	bool has_country_military_unit(const domain *domain) const;
	Q_INVOKABLE QVariantList get_country_military_unit_category_counts(metternich::domain *domain) const;
	Q_INVOKABLE int get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::domain *domain) const;

	Q_INVOKABLE const icon *get_military_unit_icon() const;
	Q_INVOKABLE const icon *get_military_unit_category_icon(const metternich::military_unit_category category) const;
	Q_INVOKABLE QString get_military_unit_category_name(const metternich::military_unit_category category) const;
	Q_INVOKABLE const icon *get_country_military_unit_icon(metternich::domain *domain) const;

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

	const std::vector<military_unit_category> &get_recruitable_military_unit_categories() const;
	QVariantList get_recruitable_military_unit_categories_qvariant_list() const;

	const military_unit_type_map<int> &get_military_unit_recruitment_counts() const;
	QVariantList get_military_unit_recruitment_counts_qvariant_list() const;

	Q_INVOKABLE int get_military_unit_recruitment_count(const metternich::military_unit_type *military_unit_type) const
	{
		const auto find_iterator = this->military_unit_recruitment_counts.find(military_unit_type);

		if (find_iterator != this->military_unit_recruitment_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_military_unit_recruitment_count(const military_unit_type *military_unit_type, const int change, const bool change_input_storage = true);
	Q_INVOKABLE bool can_increase_military_unit_recruitment(const metternich::military_unit_type *military_unit_type) const;
	Q_INVOKABLE void increase_military_unit_recruitment(const metternich::military_unit_type *military_unit_type);
	Q_INVOKABLE bool can_decrease_military_unit_recruitment(const metternich::military_unit_type *military_unit_type) const;
	Q_INVOKABLE void decrease_military_unit_recruitment(const metternich::military_unit_type *military_unit_type, const bool restore_inputs);
	void clear_military_unit_recruitment_counts();

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

		this->calculate_site_commodity_outputs();
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

	int get_min_income() const;
	int get_max_income() const;

	int get_skill_modifier(const skill *skill) const;

	Q_INVOKABLE const metternich::domain *get_trade_zone_domain() const;
	Q_INVOKABLE const metternich::domain *get_temple_domain() const;

	province_game_data &operator =(const province_game_data &other) = delete;

signals:
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void level_changed();
	void provincial_capital_changed();
	void map_image_changed();
	void attribute_values_changed();
	void visible_sites_changed();
	void technologies_changed();
	void scripted_modifiers_changed();
	void population_units_changed();
	void military_units_changed();
	void military_unit_category_counts_changed();
	void entering_armies_changed();
	void military_unit_recruitment_counts_changed();

private:
	const metternich::province *province = nullptr;
	const domain *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	int level = 0;
	const site *provincial_capital = nullptr;
	QImage map_image;
	QImage selected_map_image;
	std::map<province_map_mode, QImage> map_mode_images;
	QRect map_image_rect;
	QRect text_rect;
	data_entry_map<province_attribute, int> attribute_values;
	int settlement_count = 0; //only includes built settlements
	technology_set technologies;
	scripted_province_modifier_map<int> scripted_modifiers;
	std::vector<population_unit *> population_units;
	qunique_ptr<metternich::population> population;
	std::vector<military_unit *> military_units;
	std::map<military_unit_category, int> military_unit_category_counts;
	std::vector<army *> entering_armies; //armies entering this province
	military_unit_type_map<int> military_unit_recruitment_counts;
	commodity_map<centesimal_int> local_commodity_outputs;
	centesimal_int output_modifier;
	int resource_output_modifier = 0;
	commodity_map<centesimal_int> commodity_output_modifiers;
	resource_map<commodity_map<int>> improved_resource_commodity_bonuses;
	commodity_map<std::map<int, int>> commodity_bonuses_for_tile_thresholds;
};

}
