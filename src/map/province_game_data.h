#pragma once

#include "database/data_entry_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "population/population_type_container.h"
#include "script/scripted_modifier_container.h"
#include "technology/technology_container.h"
#include "unit/military_unit_type_container.h"
#include "util/centesimal_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("culture/culture.h")
Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("infrastructure/pathway.h")
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
class military_unit;
class pathway;
class phenotype;
class population;
class population_unit;
class province;
class religion;
class scripted_province_modifier;
class site;
class site_feature;
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
	Q_PROPERTY(const metternich::domain* trade_zone_domain READ get_trade_zone_domain NOTIFY trade_zone_domain_changed)
	Q_PROPERTY(const metternich::domain* temple_domain READ get_temple_domain NOTIFY temple_domain_changed)
	Q_PROPERTY(int level READ get_level NOTIFY level_changed)
	Q_PROPERTY(int max_level READ get_max_level NOTIFY max_level_changed)
	Q_PROPERTY(QColor map_color READ get_map_color NOTIFY owner_changed)
	Q_PROPERTY(QRect map_image_rect READ get_map_image_rect NOTIFY map_image_changed)
	Q_PROPERTY(QRect text_rect READ get_text_rect NOTIFY map_image_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(bool coastal READ is_coastal CONSTANT)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect CONSTANT)
	Q_PROPERTY(const metternich::site* provincial_capital READ get_provincial_capital NOTIFY provincial_capital_changed)
	Q_PROPERTY(QPoint center_tile_pos READ get_center_tile_pos CONSTANT)
	Q_PROPERTY(const metternich::pathway* pathway READ get_pathway NOTIFY pathway_changed)
	Q_PROPERTY(const metternich::pathway* under_construction_pathway READ get_under_construction_pathway WRITE set_under_construction_pathway NOTIFY under_construction_pathway_changed)
	Q_PROPERTY(QVariantList visible_sites READ get_visible_sites_qvariant_list NOTIFY visible_sites_changed)
	Q_PROPERTY(QVariantList dungeon_sites READ get_dungeon_sites_qvariant_list NOTIFY dungeon_sites_changed)
	Q_PROPERTY(QVariantList technologies READ get_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(QVariantList military_units READ get_military_units_qvariant_list NOTIFY military_units_changed)
	Q_PROPERTY(QVariantList military_unit_category_counts READ get_military_unit_category_counts_qvariant_list NOTIFY military_unit_category_counts_changed)
	Q_PROPERTY(QVariantList entering_armies READ get_entering_armies_qvariant_list NOTIFY entering_armies_changed)
	Q_PROPERTY(QVariantList recruitable_military_unit_categories READ get_recruitable_military_unit_categories_qvariant_list NOTIFY owner_changed)
	Q_PROPERTY(QVariantList military_unit_recruitment_counts READ get_military_unit_recruitment_counts_qvariant_list NOTIFY military_unit_recruitment_counts_changed)
	Q_PROPERTY(QVariantList civilian_units READ get_civilian_units_qvariant_list NOTIFY civilian_units_changed)
	Q_PROPERTY(int min_income READ get_min_income NOTIFY income_changed)
	Q_PROPERTY(int max_income READ get_max_income NOTIFY income_changed)

public:
	explicit province_game_data(const metternich::province *province);
	province_game_data(const province_game_data &other) = delete;
	~province_game_data();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	[[nodiscard]] QCoro::Task<void> initialize();
	[[nodiscard]] QCoro::Task<void> do_turn();
	[[nodiscard]] QCoro::Task<void> do_events();
	void do_ai_turn();
	void collect_taxes();
	[[nodiscard]] QCoro::Task<void> do_military_unit_recruitment();
	[[nodiscard]] QCoro::Task<void> do_construction(const decimillesimal_int &construction_per_project);
	void do_population_literacy_change();

	bool is_on_map() const;

	const domain *get_owner() const
	{
		return this->owner;
	}

	[[nodiscard]] QCoro::Task<void> set_owner(const domain *domain);

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

	const metternich::domain *get_trade_zone_domain() const
	{
		return this->trade_zone_domain;
	}

	void set_trade_zone_domain(const metternich::domain *trade_zone_domain);
	void check_trade_zone_domain();
	void check_trade_zone_domain_for_province_and_neighbors();

	const metternich::domain *get_temple_domain() const
	{
		return this->temple_domain;
	}

	void set_temple_domain(const metternich::domain *temple_domain);
	void check_temple_domain();
	void check_temple_domain_for_province_and_neighbors();

	int get_level() const
	{
		return this->level;
	}

	void set_level(const int level);
	void change_level(const int change);
	int get_max_level() const;
	void set_max_level(const int level);
	void change_max_level(const int change);

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

	const metternich::pathway *get_pathway() const
	{
		return this->pathway;
	}

	[[nodiscard]] QCoro::Task<void> set_pathway(const pathway *pathway);

	const pathway *get_under_construction_pathway() const
	{
		return this->under_construction_pathway;
	}

	void set_under_construction_pathway(const pathway *pathway);
	bool has_pathway_or_better(const pathway *pathway) const;
	bool can_build_pathway(const pathway *pathway) const;
	Q_INVOKABLE void build_pathway(const metternich::pathway *pathway);
	Q_INVOKABLE void cancel_pathway_construction();
	Q_INVOKABLE const metternich::pathway *get_buildable_pathway() const;

	const decimillesimal_int &get_pathway_construction_progress() const;
	Q_INVOKABLE qint64 get_pathway_construction_progress_commodity_quantity() const;
	Q_INVOKABLE QString get_pathway_construction_progress_qstring() const;
	void change_pathway_construction_progress(const decimillesimal_int &change);

	const std::vector<QPoint> &get_resource_tiles() const;
	const std::vector<const site *> &get_sites() const;
	const std::vector<const site *> &get_settlement_sites() const;

	const QColor &get_map_color() const;

	const QPromise<QImage> *get_map_image_promise() const
	{
		return this->map_image_promise.get();
	}

	QImage prepare_map_image() const;
	[[nodiscard]] static QImage finalize_map_image(QImage &&image);

	void create_map_image();

	const QPromise<QImage> *get_selected_map_image_promise() const
	{
		return this->selected_map_image_promise.get();
	}

	const QPromise<QImage> *get_interactive_map_image_promise() const
	{
		return this->interactive_map_image_promise.get();
	}

	const QPromise<QImage> *get_map_mode_image_promise(const province_map_mode mode) const;

	void create_map_mode_image(const province_map_mode mode);

	const QRect &get_map_image_rect() const
	{
		return this->map_image_rect;
	}

	const QRect &get_text_rect() const
	{
		return this->text_rect;
	}

	void calculate_text_rect();

	int get_settlement_count() const
	{
		return this->settlement_count;
	}

	void change_settlement_count(const int change)
	{
		this->settlement_count += change;
	}

	int get_total_holding_level() const
	{
		return this->total_holding_level;
	}

	void change_total_holding_level(const int change)
	{
		this->total_holding_level += change;
	}

	const resource_map<int> &get_resource_counts() const;

	const data_entry_map<site_feature, int> &get_site_feature_counts() const
	{
		return this->site_feature_counts;
	}

	void change_site_feature_count(const site_feature *feature, const int change)
	{
		if (change == 0) {
			return;
		}

		const int new_value = (this->site_feature_counts[feature] += change);

		if (new_value == 0) {
			this->site_feature_counts.erase(feature);
		}
	}

	const terrain_type *get_terrain() const;

	std::vector<const site *> get_visible_sites() const;
	QVariantList get_visible_sites_qvariant_list() const;
	QVariantList get_dungeon_sites_qvariant_list() const;

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

	[[nodiscard]] QCoro::Task<void> add_technology(const technology *technology);
	[[nodiscard]] QCoro::Task<void> add_technology_with_prerequisites(const technology *technology);
	[[nodiscard]] QCoro::Task<void> remove_technology(const technology *technology);
	bool can_gain_technology(const technology *technology) const;
	[[nodiscard]] QCoro::Task<void> on_technology_gained(const technology *technology, const int multiplier);
	centesimal_int get_extra_technology(const technology *technology) const;

	const scripted_province_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_province_modifier *modifier) const;
	[[nodiscard]] QCoro::Task<void> add_scripted_modifier(const scripted_province_modifier *modifier, const int duration);
	[[nodiscard]] QCoro::Task<void> remove_scripted_modifier(const scripted_province_modifier *modifier);
	[[nodiscard]] QCoro::Task<void> decrement_scripted_modifiers();

	[[nodiscard]] QCoro::Task<void> apply_modifier(const modifier<const metternich::province> *modifier, const int multiplier = 1);

	[[nodiscard]] QCoro::Task<void> remove_modifier(const modifier<const metternich::province> *modifier)
	{
		co_await this->apply_modifier(modifier, -1);
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

	[[nodiscard]] QCoro::Task<void> on_population_type_size_changed(const population_type *population_type, const int64_t change);

	const centesimal_int &get_population_type_modifier_multiplier(const population_type *population_type) const
	{
		const auto find_iterator = this->population_type_modifier_multipliers.find(population_type);

		if (find_iterator != this->population_type_modifier_multipliers.end()) {
			return find_iterator->second;
		}

		static const centesimal_int one(1);
		return one;
	}

	[[nodiscard]] QCoro::Task<void> set_population_type_modifier_multiplier(const population_type *population_type, const centesimal_int &value);

	[[nodiscard]] QCoro::Task<void> change_population_type_modifier_multiplier(const population_type *population_type, const centesimal_int &change)
	{
		co_await this->set_population_type_modifier_multiplier(population_type, this->get_population_type_modifier_multiplier(population_type) + change);
	}


	int64_t get_employment_capacity_modifier(const employment_type *employment_type) const
	{
		const auto find_iterator = this->employment_capacity_modifiers.find(employment_type);

		if (find_iterator != this->employment_capacity_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_employment_capacity_modifier(const employment_type *employment_type, const int64_t modifier);

	void change_employment_capacity_modifier(const employment_type *employment_type, const int64_t change)
	{
		this->set_employment_capacity_modifier(employment_type, this->get_employment_capacity_modifier(employment_type) + change);
	}

	const std::vector<military_unit *> &get_military_units() const
	{
		return this->military_units;
	}

	QVariantList get_military_units_qvariant_list() const;
	std::vector<military_unit *> get_domain_military_units(const domain *domain) const;
	Q_INVOKABLE QVariantList get_domain_military_units_qvariant_list(const metternich::domain *domain) const;

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

	bool has_domain_military_unit(const domain *domain) const;
	Q_INVOKABLE QVariantList get_domain_military_unit_category_counts(metternich::domain *domain) const;
	Q_INVOKABLE int get_domain_military_unit_category_count(const metternich::military_unit_category category, metternich::domain *domain) const;

	Q_INVOKABLE const icon *get_military_unit_icon() const;
	Q_INVOKABLE const icon *get_military_unit_category_icon(const metternich::military_unit_category category) const;
	Q_INVOKABLE QString get_military_unit_category_name(const metternich::military_unit_category category) const;
	Q_INVOKABLE const icon *get_domain_military_unit_icon(metternich::domain *domain) const;

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

	const std::vector<civilian_unit *> &get_civilian_units() const;
	QVariantList get_civilian_units_qvariant_list() const;
	void add_civilian_unit(civilian_unit *civilian_unit);
	void remove_civilian_unit(civilian_unit *civilian_unit);

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

	const commodity_map<int> &get_commodity_throughput_modifiers() const
	{
		return this->commodity_throughput_modifiers;
	}

	int get_commodity_throughput_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_throughput_modifiers.find(commodity);

		if (find_iterator != this->commodity_throughput_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_throughput_modifier(const commodity *commodity, const int value);

	void change_commodity_throughput_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_throughput_modifier(commodity, this->get_commodity_throughput_modifier(commodity) + value);
	}

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

	int get_technology_category_spread_modifier(const technology_category *category) const
	{
		const auto find_iterator = this->technology_category_spread_modifiers.find(category);

		if (find_iterator != this->technology_category_spread_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_technology_category_spread_modifier(const technology_category *category, const int change)
	{
		const int new_value = (this->technology_category_spread_modifiers[category] += change);

		if (new_value == 0) {
			this->technology_category_spread_modifiers.erase(category);
		}
	}

	int get_movement_cost_modifier() const
	{
		return this->movement_cost_modifier;
	}

	void change_movement_cost_modifier(const int change)
	{
		this->movement_cost_modifier += change;
	}

	bool can_produce_commodity(const commodity *commodity) const;

	int64_t get_min_income() const;
	int64_t get_max_income() const;

	province_game_data &operator =(const province_game_data &other) = delete;

signals:
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void trade_zone_domain_changed();
	void temple_domain_changed();
	void level_changed();
	void max_level_changed();
	void provincial_capital_changed();
	void pathway_changed();
	void under_construction_pathway_changed();
	void map_image_changed();
	void map_mode_image_changed(QString map_mode_identifier);
	void visible_sites_changed();
	void dungeon_sites_changed();
	void technologies_changed();
	void scripted_modifiers_changed();
	void population_units_changed();
	void military_units_changed();
	void military_unit_category_counts_changed();
	void entering_armies_changed();
	void military_unit_recruitment_counts_changed();
	void civilian_units_changed();
	void income_changed();

private:
	const metternich::province *province = nullptr;
	const domain *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const domain *trade_zone_domain = nullptr;
	const domain *temple_domain = nullptr;
	int level = 0;
	int max_level = 0;
	const site *provincial_capital = nullptr;
	const metternich::pathway *pathway = nullptr;
	const metternich::pathway *under_construction_pathway = nullptr;
	decimillesimal_int pathway_construction_progress;
	std::shared_ptr<QPromise<QImage>> map_image_promise;
	std::shared_ptr<QPromise<QImage>> selected_map_image_promise;
	std::shared_ptr<QPromise<QImage>> interactive_map_image_promise;
	std::map<province_map_mode, std::shared_ptr<QPromise<QImage>>> map_mode_image_promises;
	QRect map_image_rect;
	QRect text_rect;
	int settlement_count = 0; //only includes built settlements
	int total_holding_level = 0;
	data_entry_map<site_feature, int> site_feature_counts;
	technology_set technologies;
	scripted_province_modifier_map<int> scripted_modifiers;
	std::vector<population_unit *> population_units;
	qunique_ptr<metternich::population> population;
	population_type_map<centesimal_int> population_type_modifier_multipliers;
	data_entry_map<employment_type, int64_t> employment_capacity_modifiers;
	std::vector<military_unit *> military_units;
	std::map<military_unit_category, int> military_unit_category_counts;
	std::vector<army *> entering_armies; //armies entering this province
	military_unit_type_map<int> military_unit_recruitment_counts;
	std::vector<civilian_unit *> civilian_units;
	commodity_map<centesimal_int> local_commodity_outputs;
	centesimal_int output_modifier;
	commodity_map<centesimal_int> commodity_output_modifiers;
	commodity_map<int> commodity_throughput_modifiers;
	commodity_map<std::map<int, int>> commodity_bonuses_for_tile_thresholds;
	data_entry_map<technology_category, int> technology_category_spread_modifiers;
	int movement_cost_modifier = 0;
};

}
