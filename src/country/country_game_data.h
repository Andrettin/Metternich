#pragma once

#include "country/consulate_container.h"
#include "country/country_container.h"
#include "country/law_group_container.h"
#include "country/policy_container.h"
#include "country/tradition_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "infrastructure/building_class_container.h"
#include "infrastructure/building_type_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "infrastructure/improvement_container.h"
#include "map/province_container.h"
#include "map/site_container.h"
#include "map/terrain_type_container.h"
#include "population/population_type_container.h"
#include "population/profession_container.h"
#include "script/opinion_modifier_container.h"
#include "technology/technology_container.h"
#include "unit/military_unit_type_container.h"
#include "unit/promotion_container.h"
#include "unit/transporter_type_container.h"
#include "util/fractional_int.h"
#include "util/point_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("country/government_type.h")
Q_MOC_INCLUDE("country/journal_entry.h")
Q_MOC_INCLUDE("country/law.h")
Q_MOC_INCLUDE("country/law_group.h")
Q_MOC_INCLUDE("country/policy.h")
Q_MOC_INCLUDE("country/religion.h")
Q_MOC_INCLUDE("country/subject_type.h")
Q_MOC_INCLUDE("country/tradition.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("population/population.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("unit/military_unit_type.h")

namespace metternich {

class army;
class building_type;
class character;
class civilian_unit;
class consulate;
class country;
class country_building_slot;
class country_rank;
class culture;
class event;
class flag;
class government_type;
class journal_entry;
class law;
class military_unit;
class military_unit_type;
class opinion_modifier;
class phenotype;
class population;
class population_class;
class population_type;
class population_unit;
class profession;
class province;
class region;
class religion;
class site;
class subject_type;
class tradition;
class transporter;
class transporter_type;
class wonder;
enum class country_tier;
enum class diplomacy_state;
enum class diplomatic_map_mode;
enum class event_trigger;
enum class income_transaction_type;
enum class military_unit_category;
enum class military_unit_stat;
enum class technology_category;
enum class transporter_category;
enum class transporter_stat;
struct read_only_context;

class country_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_tier tier READ get_tier NOTIFY tier_changed)
	Q_PROPERTY(QString name READ get_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString title_name READ get_title_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString ruler_title_name READ get_ruler_title_name_qstring NOTIFY ruler_title_name_changed)
	Q_PROPERTY(const metternich::religion* religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(const metternich::country* overlord READ get_overlord NOTIFY overlord_changed)
	Q_PROPERTY(QString type_name READ get_type_name_qstring NOTIFY type_name_changed)
	Q_PROPERTY(const metternich::subject_type* subject_type READ get_subject_type NOTIFY subject_type_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(const metternich::site* capital READ get_capital NOTIFY capital_changed)
	Q_PROPERTY(bool coastal READ is_coastal NOTIFY provinces_changed)
	Q_PROPERTY(bool anarchy READ is_under_anarchy NOTIFY provinces_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QPoint territory_rect_center READ get_territory_rect_center NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList contiguous_territory_rects READ get_contiguous_territory_rects_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QRect main_contiguous_territory_rect READ get_main_contiguous_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QRect text_rect READ get_text_rect NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList resource_counts READ get_resource_counts_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList vassal_resource_counts READ get_vassal_resource_counts_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList tile_terrain_counts READ get_tile_terrain_counts_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList vassals READ get_vassals_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList subject_type_counts READ get_subject_type_counts_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList consulates READ get_consulates_qvariant_list NOTIFY consulates_changed)
	Q_PROPERTY(QRect diplomatic_map_image_rect READ get_diplomatic_map_image_rect NOTIFY diplomatic_map_image_changed)
	Q_PROPERTY(int score READ get_score NOTIFY score_changed)
	Q_PROPERTY(int score_rank READ get_score_rank NOTIFY score_rank_changed)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(int population_growth READ get_population_growth NOTIFY population_growth_changed)
	Q_PROPERTY(int health READ get_health_int NOTIFY health_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(int wealth READ get_wealth NOTIFY wealth_changed)
	Q_PROPERTY(int wealth_income READ get_wealth_income NOTIFY wealth_income_changed)
	Q_PROPERTY(int credit_limit READ get_credit_limit NOTIFY credit_limit_changed)
	Q_PROPERTY(QString inflation READ get_inflation_qstring NOTIFY inflation_changed)
	Q_PROPERTY(QVariantList available_commodities READ get_available_commodities_qvariant_list NOTIFY available_commodities_changed)
	Q_PROPERTY(QVariantList tradeable_commodities READ get_tradeable_commodities_qvariant_list NOTIFY tradeable_commodities_changed)
	Q_PROPERTY(QVariantList stored_commodities READ get_stored_commodities_qvariant_list NOTIFY stored_commodities_changed)
	Q_PROPERTY(int storage_capacity READ get_storage_capacity NOTIFY storage_capacity_changed)
	Q_PROPERTY(QVariantList commodity_inputs READ get_commodity_inputs_qvariant_list NOTIFY commodity_inputs_changed)
	Q_PROPERTY(QVariantList transportable_commodity_outputs READ get_transportable_commodity_outputs_qvariant_list NOTIFY transportable_commodity_outputs_changed)
	Q_PROPERTY(QVariantList transported_commodity_outputs READ get_transported_commodity_outputs_qvariant_list NOTIFY transported_commodity_outputs_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)
	Q_PROPERTY(QVariantList everyday_consumption READ get_everyday_consumption_qvariant_list NOTIFY everyday_consumption_changed)
	Q_PROPERTY(QVariantList luxury_consumption READ get_luxury_consumption_qvariant_list NOTIFY luxury_consumption_changed)
	Q_PROPERTY(int land_transport_capacity READ get_land_transport_capacity NOTIFY land_transport_capacity_changed)
	Q_PROPERTY(int sea_transport_capacity READ get_sea_transport_capacity NOTIFY sea_transport_capacity_changed)
	Q_PROPERTY(QVariantList technologies READ get_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList available_technologies READ get_available_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList future_technologies READ get_future_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(const metternich::technology* current_research READ get_current_research WRITE set_current_research NOTIFY current_research_changed)
	Q_PROPERTY(int research_cost_modifier READ get_research_cost_modifier NOTIFY provinces_changed)
	Q_PROPERTY(QColor diplomatic_map_color READ get_diplomatic_map_color NOTIFY overlord_changed)
	Q_PROPERTY(const metternich::government_type* government_type READ get_government_type NOTIFY government_type_changed)
	Q_PROPERTY(QVariantList laws READ get_laws_qvariant_list NOTIFY laws_changed)
	Q_PROPERTY(QVariantList policy_values READ get_policy_values_qvariant_list NOTIFY policy_values_changed)
	Q_PROPERTY(QVariantList available_traditions READ get_available_traditions_qvariant_list NOTIFY traditions_changed)
	Q_PROPERTY(int tradition_cost READ get_tradition_cost NOTIFY traditions_changed)
	Q_PROPERTY(const metternich::tradition* next_tradition READ get_next_tradition WRITE set_next_tradition NOTIFY next_tradition_changed)
	Q_PROPERTY(const metternich::tradition* next_belief READ get_next_belief WRITE set_next_belief NOTIFY next_belief_changed)
	Q_PROPERTY(const metternich::character* ruler READ get_ruler NOTIFY ruler_changed)
	Q_PROPERTY(QVariantList advisors READ get_advisors_qvariant_list NOTIFY advisors_changed)
	Q_PROPERTY(int advisor_cost READ get_advisor_cost NOTIFY advisors_changed)
	Q_PROPERTY(const metternich::character* next_advisor READ get_next_advisor WRITE set_next_advisor NOTIFY next_advisor_changed)
	Q_PROPERTY(QVariantList leaders READ get_leaders_qvariant_list NOTIFY leaders_changed)
	Q_PROPERTY(int leader_cost READ get_leader_cost NOTIFY leaders_changed)
	Q_PROPERTY(const metternich::character* next_leader READ get_next_leader WRITE set_next_leader NOTIFY next_leader_changed)
	Q_PROPERTY(const metternich::military_unit_type* next_leader_military_unit_type READ get_next_leader_military_unit_type NOTIFY next_leader_changed)
	Q_PROPERTY(QVariantList bids READ get_bids_qvariant_list NOTIFY bids_changed)
	Q_PROPERTY(QVariantList offers READ get_offers_qvariant_list NOTIFY offers_changed)
	Q_PROPERTY(int output_modifier READ get_output_modifier NOTIFY output_modifier_changed)
	Q_PROPERTY(int resource_output_modifier READ get_resource_output_modifier NOTIFY resource_output_modifier_changed)
	Q_PROPERTY(int industrial_output_modifier READ get_industrial_output_modifier NOTIFY industrial_output_modifier_changed)
	Q_PROPERTY(int throughput_modifier READ get_throughput_modifier NOTIFY throughput_modifier_changed)
	Q_PROPERTY(QVariantList active_journal_entries READ get_active_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(QVariantList inactive_journal_entries READ get_inactive_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(QVariantList finished_journal_entries READ get_finished_journal_entries_qvariant_list NOTIFY journal_entries_changed)

public:
	static constexpr int base_advisor_cost = 80;
	static constexpr int base_leader_cost = 80;
	static constexpr int base_deployment_limit = 10;
	static constexpr int vassal_tax_rate = 50;

	explicit country_game_data(metternich::country *country);
	~country_game_data();

	void do_turn();
	void do_production();
	void do_research();
	void do_population_growth();
	void do_food_consumption(const int food_consumption);
	void do_starvation();
	void do_everyday_consumption();
	void do_luxury_consumption();
	void do_cultural_change();
	void do_construction();
	void do_trade(country_map<commodity_map<int>> &country_luxury_demands);
	void do_inflation();
	void do_events();
	void do_ai_turn();

	bool is_ai() const;

	country_tier get_tier() const
	{
		return this->tier;
	}

	void set_tier(const country_tier tier);

	const std::string &get_name() const;

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	std::string get_titled_name() const;

	QString get_titled_name_qstring() const
	{
		return QString::fromStdString(this->get_titled_name());
	}

	const std::string &get_title_name() const;

	QString get_title_name_qstring() const
	{
		return QString::fromStdString(this->get_title_name());
	}

	const std::string &get_ruler_title_name() const;

	QString get_ruler_title_name_qstring() const
	{
		return QString::fromStdString(this->get_ruler_title_name());
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	void set_religion(const metternich::religion *religion);

	const metternich::country *get_overlord() const
	{
		return this->overlord;
	}

	void set_overlord(const metternich::country *overlord);

	bool is_vassal_of(const metternich::country *country) const;
	bool is_any_vassal_of(const metternich::country *country) const;

	Q_INVOKABLE bool is_any_vassal_of(metternich::country *country)
	{
		const metternich::country *country_const = country;
		return this->is_any_vassal_of(country_const);
	}

	bool is_overlord_of(const metternich::country *country) const;
	bool is_any_overlord_of(const metternich::country *country) const;

	bool is_independent() const
	{
		return this->get_overlord() == nullptr;
	}

	std::string get_type_name() const;

	QString get_type_name_qstring() const
	{
		return QString::fromStdString(this->get_type_name());
	}

	const metternich::subject_type *get_subject_type() const
	{
		return this->subject_type;
	}

	void set_subject_type(const metternich::subject_type *subject_type);

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;
	void add_province(const province *province);
	void remove_province(const province *province);
	void on_province_gained(const province *province, const int multiplier);

	int get_province_count() const
	{
		return static_cast<int>(this->get_provinces().size());
	}

	void on_site_gained(const site *site, const int multiplier);

	const site *get_capital() const
	{
		return this->capital;
	}

	void set_capital(const site *capital);
	void choose_capital();

	const province *get_capital_province() const;

	int get_settlement_count() const
	{
		return this->settlement_count;
	}

	void change_settlement_count(const int change);

	const std::vector<const province *> &get_border_provinces() const
	{
		return this->border_provinces;
	}

	bool is_alive() const
	{
		return !this->get_provinces().empty();
	}

	bool is_under_anarchy() const
	{
		return this->get_capital() == nullptr;
	}

	bool is_coastal() const
	{
		return this->coastal_province_count > 0;
	}

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	void calculate_territory_rect();

	const QPoint &get_territory_rect_center() const
	{
		return this->territory_rect_center;
	}

	void calculate_territory_rect_center();

	const std::vector<QRect> &get_contiguous_territory_rects() const
	{
		return this->contiguous_territory_rects;
	}

	QVariantList get_contiguous_territory_rects_qvariant_list() const;

	const QRect &get_main_contiguous_territory_rect() const
	{
		return this->main_contiguous_territory_rect;
	}

	const QRect &get_text_rect() const
	{
		return this->text_rect;
	}

	void calculate_text_rect();

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

	QVariantList get_resource_counts_qvariant_list() const;

	void change_resource_count(const resource *resource, const int change)
	{
		const int final_count = (this->resource_counts[resource] += change);

		if (final_count == 0) {
			this->resource_counts.erase(resource);
		}
	}

	const resource_map<int> &get_vassal_resource_counts() const
	{
		return this->vassal_resource_counts;
	}

	QVariantList get_vassal_resource_counts_qvariant_list() const;

	void change_vassal_resource_count(const resource *resource, const int change)
	{
		const int final_count = (this->vassal_resource_counts[resource] += change);

		if (final_count == 0) {
			this->vassal_resource_counts.erase(resource);
		}
	}

	const terrain_type_map<int> &get_tile_terrain_counts() const
	{
		return this->tile_terrain_counts;
	}

	QVariantList get_tile_terrain_counts_qvariant_list() const;

	void change_tile_terrain_count(const terrain_type *terrain, const int change)
	{
		const int final_count = (this->tile_terrain_counts[terrain] += change);

		if (final_count == 0) {
			this->tile_terrain_counts.erase(terrain);
		}
	}

	const country_set &get_known_countries() const
	{
		return this->known_countries;
	}

	bool is_country_known(const metternich::country *other_country) const
	{
		return this->get_known_countries().contains(other_country);
	}

	void add_known_country(const metternich::country *other_country);

	void remove_known_country(const metternich::country *other_country)
	{
		this->known_countries.erase(other_country);
	}

	diplomacy_state get_diplomacy_state(const metternich::country *other_country) const;
	void set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state);

	const std::map<diplomacy_state, int> &get_diplomacy_state_counts() const
	{
		return this->diplomacy_state_counts;
	}

	void change_diplomacy_state_count(const diplomacy_state state, const int change);
	Q_INVOKABLE QString get_diplomacy_state_diplomatic_map_suffix(metternich::country *other_country) const;

	bool at_war() const;

	bool can_attack(const metternich::country *other_country) const;

	std::optional<diplomacy_state> get_offered_diplomacy_state(const metternich::country *other_country) const;

	Q_INVOKABLE int get_offered_diplomacy_state_int(metternich::country *other_country) const
	{
		const std::optional<diplomacy_state> state = this->get_offered_diplomacy_state(other_country);

		if (!state.has_value()) {
			return -1;
		}

		return static_cast<int>(state.value());
	}

	void set_offered_diplomacy_state(const metternich::country *other_country, const std::optional<diplomacy_state> &state);

	Q_INVOKABLE void set_offered_diplomacy_state_int(metternich::country *other_country, const int state)
	{
		if (state == -1) {
			this->set_offered_diplomacy_state(other_country, std::nullopt);
		} else {
			this->set_offered_diplomacy_state(other_country, static_cast<diplomacy_state>(state));
		}
	}

	QVariantList get_consulates_qvariant_list() const;

	const consulate *get_consulate(const metternich::country *other_country) const
	{
		const auto find_iterator = this->consulates.find(other_country);

		if (find_iterator != this->consulates.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_consulate(const metternich::country *other_country, const consulate *consulate);

	int get_opinion_of(const metternich::country *other) const;

	int get_base_opinion(const metternich::country *other) const
	{
		const auto find_iterator = this->base_opinions.find(other);
		if (find_iterator != this->base_opinions.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_base_opinion(const metternich::country *other, const int opinion);

	void change_base_opinion(const metternich::country *other, const int change)
	{
		this->set_base_opinion(other, this->get_base_opinion(other) + change);
	}

	const opinion_modifier_map<int> &get_opinion_modifiers_for(const metternich::country *other) const
	{
		static const opinion_modifier_map<int> empty_map;

		const auto find_iterator = this->opinion_modifiers.find(other);
		if (find_iterator != this->opinion_modifiers.end()) {
			return find_iterator->second;
		}

		return empty_map;
	}

	void add_opinion_modifier(const metternich::country *other, const opinion_modifier *modifier, const int duration);
	void remove_opinion_modifier(const metternich::country *other, const opinion_modifier *modifier);

	int get_opinion_weighted_prestige_for(const metternich::country *other) const;

	std::vector<const metternich::country *> get_vassals() const;
	QVariantList get_vassals_qvariant_list() const;
	QVariantList get_subject_type_counts_qvariant_list() const;

	std::vector<const metternich::country *> get_neighbor_countries() const;

	const QColor &get_diplomatic_map_color() const;

	const QImage &get_diplomatic_map_image() const
	{
		return this->diplomatic_map_image;
	}

	QImage prepare_diplomatic_map_image() const;

	[[nodiscard]]
	QCoro::Task<QImage> finalize_diplomatic_map_image(QImage &&image) const;

	[[nodiscard]]
	QCoro::Task<void> create_diplomatic_map_image();

	const QRect &get_diplomatic_map_image_rect() const
	{
		return this->diplomatic_map_image_rect;
	}

	const QImage &get_selected_diplomatic_map_image() const
	{
		return this->selected_diplomatic_map_image;
	}

	const QImage &get_diplomatic_map_mode_image(const diplomatic_map_mode mode) const
	{
		const auto find_iterator = this->diplomatic_map_mode_images.find(mode);
		if (find_iterator != this->diplomatic_map_mode_images.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("No diplomatic map image found for mode " + std::to_string(static_cast<int>(mode)) + ".");
	}

	[[nodiscard]]
	QCoro::Task<void> create_diplomatic_map_mode_image(const diplomatic_map_mode mode);

	const QImage &get_diplomacy_state_diplomatic_map_image(const diplomacy_state state) const
	{
		const auto find_iterator = this->diplomacy_state_diplomatic_map_images.find(state);
		if (find_iterator != this->diplomacy_state_diplomatic_map_images.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("No diplomacy state diplomatic map image found for state " + std::to_string(static_cast<int>(state)) + ".");
	}

	[[nodiscard]]
	QCoro::Task<void> create_diplomacy_state_diplomatic_map_image(const diplomacy_state state);

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change);

	int get_economic_score() const
	{
		return this->economic_score;
	}

	void change_economic_score(const int change);

	int get_military_score() const
	{
		return this->military_score;
	}

	void change_military_score(const int change);

	int get_score_rank() const
	{
		return this->score_rank;
	}

	void set_score_rank(const int score_rank)
	{
		if (score_rank == this->get_score_rank()) {
			return;
		}

		this->score_rank = score_rank;
		emit score_rank_changed();
	}

	const country_rank *get_rank() const
	{
		return this->rank;
	}

	void set_rank(const country_rank *rank)
	{
		if (rank == this->get_rank()) {
			return;
		}

		this->rank = rank;
		emit rank_changed();
	}

	const population_class *get_default_population_class() const;

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

	metternich::population *get_population() const
	{
		return this->population.get();
	}

	void on_population_type_count_changed(const population_type *type, const int change);

	int get_population_growth() const
	{
		return this->population_growth;
	}

	void set_population_growth(const int growth);

	void change_population_growth(const int change)
	{
		this->set_population_growth(this->get_population_growth() + change);
	}

	void grow_population();
	void decrease_population();
	population_unit *choose_starvation_population_unit();

	Q_INVOKABLE const icon *get_population_type_small_icon(const metternich::population_type *type) const;

	int get_total_unit_count() const
	{
		return this->get_population_unit_count() + static_cast<int>(this->civilian_units.size()) + static_cast<int>(this->military_units.size()) + static_cast<int>(this->transporters.size());
	}

	const centesimal_int &get_health() const
	{
		return this->health;
	}

	int get_health_int() const
	{
		return this->get_health().to_int();
	}

	void change_health(const centesimal_int &change)
	{
		if (change == 0) {
			return;
		}

		this->health += change;

		emit health_changed();
	}

	centesimal_int get_available_health() const
	{
		return this->get_health() - this->get_population_unit_count();
	}

	int get_food_consumption() const
	{
		return this->food_consumption;
	}

	void change_food_consumption(const int change)
	{
		this->food_consumption += change;
	}

	int get_net_food_consumption() const;

	int get_available_food() const
	{
		return this->get_stored_food() - this->get_net_food_consumption();
	}

	QVariantList get_building_slots_qvariant_list() const;
	void initialize_building_slots();

	country_building_slot *get_building_slot(const building_slot_type *slot_type) const
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
	void clear_buildings();

	int get_settlement_building_count(const building_type *building) const
	{
		const auto find_iterator = this->settlement_building_counts.find(building);

		if (find_iterator != this->settlement_building_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_settlement_building_count(metternich::building_type *building) const
	{
		const metternich::building_type *const_building = building;
		return this->get_settlement_building_count(const_building);
	}

	void change_settlement_building_count(const building_type *building, const int change);

	void on_wonder_gained(const wonder *wonder, const int multiplier);

	int get_wealth() const
	{
		return this->wealth;
	}

	void set_wealth(const int wealth)
	{
		if (wealth == this->get_wealth()) {
			return;
		}

		this->wealth = wealth;

		emit wealth_changed();
	}

	void change_wealth(const int change)
	{
		this->set_wealth(this->get_wealth() + change);
	}

	void change_wealth_inflated(const int change)
	{
		this->change_wealth(this->get_inflated_value(change));
	}

	void add_taxable_wealth(const int taxable_wealth, const income_transaction_type tax_income_type);

	int get_wealth_income() const
	{
		return this->wealth_income;
	}

	void set_wealth_income(const int income)
	{
		if (income == this->get_wealth_income()) {
			return;
		}

		this->change_economic_score(-this->get_wealth_income());

		this->wealth_income = income;

		this->change_economic_score(this->get_wealth_income());

		emit wealth_income_changed();
	}

	void change_wealth_income(const int change)
	{
		this->set_wealth_income(this->get_wealth_income() + change);
	}

	int get_credit_limit() const
	{
		return this->credit_limit;
	}

	void set_credit_limit(const int credit_limit)
	{
		if (credit_limit == this->get_credit_limit()) {
			return;
		}

		this->credit_limit = credit_limit;

		emit credit_limit_changed();
	}

	void change_credit_limit(const int change)
	{
		this->set_credit_limit(this->get_credit_limit() + change);
	}

	int get_wealth_with_credit() const
	{
		return this->get_wealth() + this->get_credit_limit();
	}

	const centesimal_int &get_inflation() const
	{
		return this->inflation;
	}

	QString get_inflation_qstring() const
	{
		return QString::fromStdString(this->get_inflation().to_string());
	}

	void set_inflation(const centesimal_int &inflation);

	void change_inflation(const centesimal_int &change)
	{
		this->set_inflation(this->get_inflation() + change);
	}

	Q_INVOKABLE int get_inflated_value(const int value) const
	{
		return (value * (centesimal_int(100) + this->get_inflation()) / 100).to_int();
	}

	const centesimal_int &get_inflation_change() const
	{
		return this->inflation_change;
	}

	void set_inflation_change(const centesimal_int &inflation_change);

	void change_inflation_change(const centesimal_int &change)
	{
		this->set_inflation_change(this->get_inflation_change() + change);
	}

	const commodity_set &get_available_commodities() const
	{
		return this->available_commodities;
	}

	QVariantList get_available_commodities_qvariant_list() const;

	void add_available_commodity(const commodity *commodity)
	{
		this->available_commodities.insert(commodity);
		emit available_commodities_changed();
	}

	const commodity_set &get_tradeable_commodities() const
	{
		return this->tradeable_commodities;
	}

	QVariantList get_tradeable_commodities_qvariant_list() const;

	void add_tradeable_commodity(const commodity *commodity)
	{
		this->tradeable_commodities.insert(commodity);
		emit tradeable_commodities_changed();
	}

	bool can_trade_commodity(const commodity *commodity) const
	{
		return this->get_tradeable_commodities().contains(commodity);
	}

	const commodity_map<int> &get_stored_commodities() const
	{
		return this->stored_commodities;
	}

	QVariantList get_stored_commodities_qvariant_list() const;

	Q_INVOKABLE int get_stored_commodity(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->stored_commodities.find(commodity);

		if (find_iterator != this->stored_commodities.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_stored_commodity(const commodity *commodity, const int value);

	void change_stored_commodity(const commodity *commodity, const int value)
	{
		this->set_stored_commodity(commodity, this->get_stored_commodity(commodity) + value);
	}

	int get_stored_food() const;

	int get_storage_capacity() const
	{
		return this->storage_capacity;
	}

	void set_storage_capacity(const int capacity);

	void change_storage_capacity(const int change)
	{
		this->set_storage_capacity(this->get_storage_capacity() + change);
	}

	const commodity_map<int> &get_commodity_inputs() const
	{
		return this->commodity_inputs;
	}

	QVariantList get_commodity_inputs_qvariant_list() const;

	int get_commodity_input(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_inputs.find(commodity);

		if (find_iterator != this->commodity_inputs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_commodity_input(const QString &commodity_identifier) const;
	void change_commodity_input(const commodity *commodity, const int change);

	const commodity_map<centesimal_int> &get_transportable_commodity_outputs() const
	{
		return this->transportable_commodity_outputs;
	}

	QVariantList get_transportable_commodity_outputs_qvariant_list() const;

	const centesimal_int &get_transportable_commodity_output(const commodity *commodity) const
	{
		const auto find_iterator = this->transportable_commodity_outputs.find(commodity);

		if (find_iterator != this->transportable_commodity_outputs.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_transportable_commodity_output(const QString &commodity_identifier) const;
	void change_transportable_commodity_output(const commodity *commodity, const centesimal_int &change);

	const commodity_map<int> &get_transported_commodity_outputs() const
	{
		return this->transported_commodity_outputs;
	}

	QVariantList get_transported_commodity_outputs_qvariant_list() const;

	Q_INVOKABLE int get_transported_commodity_output(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->transported_commodity_outputs.find(commodity);

		if (find_iterator != this->transported_commodity_outputs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE void change_transported_commodity_output(const metternich::commodity *commodity, const int change);

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

	Q_INVOKABLE int get_commodity_output(const QString &commodity_identifier) const;
	void change_commodity_output(const commodity *commodity, const centesimal_int &change);

	int get_net_commodity_output(const commodity *commodity) const
	{
		return this->get_commodity_output(commodity).to_int() - this->get_commodity_input(commodity);
	}

	void calculate_site_commodity_outputs();
	void calculate_site_commodity_output(const commodity *commodity);

	int get_everyday_wealth_consumption() const
	{
		return this->everyday_wealth_consumption;
	}

	void change_everyday_wealth_consumption(const int change);

	const commodity_map<centesimal_int> &get_everyday_consumption() const
	{
		return this->everyday_consumption;
	}

	QVariantList get_everyday_consumption_qvariant_list() const;

	const centesimal_int &get_everyday_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->everyday_consumption.find(commodity);

		if (find_iterator != this->everyday_consumption.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_everyday_consumption(const QString &commodity_identifier) const;
	void change_everyday_consumption(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_luxury_consumption() const
	{
		return this->luxury_consumption;
	}

	QVariantList get_luxury_consumption_qvariant_list() const;

	const centesimal_int &get_luxury_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->luxury_consumption.find(commodity);

		if (find_iterator != this->luxury_consumption.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_luxury_consumption(const QString &commodity_identifier) const;
	void change_luxury_consumption(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_commodity_demands() const
	{
		return this->commodity_demands;
	}

	const centesimal_int &get_commodity_demand(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_demands.find(commodity);

		if (find_iterator != this->commodity_demands.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	void change_commodity_demand(const commodity *commodity, const centesimal_int &change);

	void assign_production();
	void decrease_wealth_consumption(const bool restore_inputs = true);
	void decrease_commodity_consumption(const commodity *commodity, const bool restore_inputs = true);

	bool produces_commodity(const commodity *commodity) const;

	int get_land_transport_capacity() const
	{
		return this->land_transport_capacity;
	}

	void set_land_transport_capacity(const int capacity);

	void change_land_transport_capacity(const int change)
	{
		this->set_land_transport_capacity(this->get_land_transport_capacity() + change);
	}

	int get_sea_transport_capacity() const
	{
		return this->sea_transport_capacity;
	}

	void set_sea_transport_capacity(const int capacity);

	void change_sea_transport_capacity(const int change)
	{
		this->set_sea_transport_capacity(this->get_sea_transport_capacity() + change);
	}

	int get_available_transport_capacity() const
	{
		const int total_capacity = this->get_land_transport_capacity() + this->get_sea_transport_capacity();
		int available_capacity = total_capacity;
		for (const auto &[commodity, transported_output] : this->get_transported_commodity_outputs()) {
			available_capacity -= transported_output;
		}
		return available_capacity;
	}

	void assign_transport_orders();

	bool can_declare_war_on(const metternich::country *other_country) const;

	const technology_set &get_technologies() const
	{
		return this->technologies;
	}

	QVariantList get_technologies_qvariant_list() const;

	bool has_technology(const technology *technology) const
	{
		return this->get_technologies().contains(technology);
	}

	Q_INVOKABLE bool has_technology(metternich::technology *technology) const
	{
		const metternich::technology *const_technology = technology;
		return this->has_technology(const_technology);
	}

	void add_technology(const technology *technology);
	void add_technology_with_prerequisites(const technology *technology);

	bool can_gain_technology(const technology *technology) const;

	std::vector<const technology *> get_available_technologies() const;
	QVariantList get_available_technologies_qvariant_list() const;
	bool is_technology_available(const technology *technology) const;

	Q_INVOKABLE bool is_technology_available(metternich::technology *technology) const
	{
		const metternich::technology *const_technology = technology;
		return this->is_technology_available(const_technology);
	}

	QVariantList get_future_technologies_qvariant_list() const;

	const technology *get_current_research() const
	{
		return this->current_research;
	}

	void set_current_research(const technology *technology);
	void choose_current_research();
	void on_technology_researched(const technology *technology);

	int get_research_cost_modifier() const
	{
		return 100 + (this->get_population_unit_count() - 1);
	}

	std::map<technology_category, const technology *> get_research_choice_map() const;
	const technology *get_ai_research_choice(const std::map<technology_category, const technology *> &research_choice_map) const;

	void gain_free_technology();

	void gain_free_technology(const technology *technology)
	{
		this->on_technology_researched(technology);
		--this->free_technology_count;

		if (this->free_technology_count > 0) {
			this->gain_free_technology();
		}
	}

	Q_INVOKABLE void gain_free_technology(metternich::technology *technology)
	{
		const metternich::technology *const_technology = technology;
		return this->gain_free_technology(const_technology);
	}

	void gain_free_technologies(const int count);
	void gain_technologies_known_by_others();

	const metternich::government_type *get_government_type() const
	{
		return this->government_type;
	}

	void set_government_type(const metternich::government_type *government_type);
	bool can_have_government_type(const metternich::government_type *government_type) const;
	void check_government_type();

	bool is_tribal() const;

	const law_group_map<const law *> &get_laws() const
	{
		return this->laws;
	}

	QVariantList get_laws_qvariant_list() const;

	Q_INVOKABLE const metternich::law *get_law(const metternich::law_group *law_group) const
	{
		const auto find_iterator = this->get_laws().find(law_group);

		if (find_iterator != this->get_laws().end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_law(const law_group *law_group, const law *law);
	bool has_law(const law *law) const;
	Q_INVOKABLE bool can_have_law(const metternich::law *law) const;
	Q_INVOKABLE bool can_enact_law(const metternich::law *law) const;
	Q_INVOKABLE void enact_law(const metternich::law *law);

	Q_INVOKABLE int get_total_law_cost_modifier() const
	{
		return 100 + (this->get_population_unit_count() - 1) + this->get_law_cost_modifier();
	}

	void check_laws();

	const policy_map<int> &get_policy_values() const
	{
		return this->policy_values;
	}

	QVariantList get_policy_values_qvariant_list() const;

	Q_INVOKABLE int get_policy_value(const metternich::policy *policy) const
	{
		const auto find_iterator = this->get_policy_values().find(policy);

		if (find_iterator != this->get_policy_values().end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_policy_value(const policy *policy, const int value);
	
	void change_policy_value(const policy *policy, const int change)
	{
		this->set_policy_value(policy, this->get_policy_value(policy) + change);
	}

	Q_INVOKABLE int get_min_policy_value(const metternich::policy *policy) const;
	Q_INVOKABLE int get_max_policy_value(const metternich::policy *policy) const;

	Q_INVOKABLE bool can_change_policy_value(const metternich::policy *policy, const int change) const;
	Q_INVOKABLE void do_policy_value_change(const metternich::policy *policy, const int change);

	Q_INVOKABLE int get_policy_value_change_cost_modifier() const
	{
		return 100 + (this->get_population_unit_count() - 1);
	}

	const tradition_set &get_traditions() const
	{
		return this->traditions;
	}

	Q_INVOKABLE bool has_tradition(const metternich::tradition *tradition) const
	{
		return this->get_traditions().contains(tradition);
	}

	std::vector<const tradition *> get_available_traditions() const;
	QVariantList get_available_traditions_qvariant_list() const;

	Q_INVOKABLE bool can_have_tradition(const metternich::tradition *tradition) const;
	void gain_tradition(const tradition *tradition, const int multiplier);
	void gain_tradition_with_prerequisites(const tradition *tradition);
	void check_traditions();
	void check_beliefs();

	int get_tradition_cost() const
	{
		int cost = 3 * static_cast<int>(this->get_traditions().size()) + 25;
		cost *= 100 + (10 * (this->get_province_count() - 1));
		cost /= 100;

		cost *= 100 + this->get_tradition_cost_modifier();
		cost /= 100;

		return std::max(0, cost);
	}

	const tradition *get_next_tradition() const
	{
		return this->next_tradition;
	}

	void set_next_tradition(const tradition *tradition)
	{
		if (tradition == this->get_next_tradition()) {
			return;
		}

		this->next_tradition = tradition;
		emit next_tradition_changed();
	}

	void choose_next_tradition();

	const tradition *get_next_belief() const
	{
		return this->next_belief;
	}

	void set_next_belief(const tradition *belief)
	{
		if (belief == this->get_next_belief()) {
			return;
		}

		this->next_belief = belief;
		emit next_belief_changed();
	}

	void choose_next_belief();

	void check_characters();

	const character *get_ruler() const
	{
		return this->ruler;
	}

	void set_ruler(const character *ruler);
	void check_ruler();

	const std::vector<const character *> &get_advisors() const
	{
		return this->advisors;
	}

	QVariantList get_advisors_qvariant_list() const;
	void check_advisors();
	void add_advisor(const character *advisor);
	void remove_advisor(const character *advisor);
	void clear_advisors();

	int get_advisor_cost() const
	{
		int cost = 0;

		if (this->get_advisors().empty()) {
			cost = country_game_data::base_advisor_cost / 2;
		} else {
			cost = country_game_data::base_advisor_cost * static_cast<int>(this->get_advisors().size() + 1);
		}

		cost *= 100 + this->get_advisor_cost_modifier();
		cost /= 100;

		return std::max(0, cost);
	}

	const character *get_next_advisor() const
	{
		return this->next_advisor;
	}

	void set_next_advisor(const character *advisor)
	{
		if (advisor == this->get_next_advisor()) {
			return;
		}

		this->next_advisor = advisor;
		emit next_advisor_changed();
	}

	void choose_next_advisor();
	bool can_have_advisors() const;
	bool can_recruit_advisor(const character *advisor) const;
	bool has_incompatible_advisor_to(const character *advisor) const;
	const character *get_replaced_advisor_for(const character *advisor) const;

	const std::vector<const character *> &get_leaders() const
	{
		return this->leaders;
	}

	QVariantList get_leaders_qvariant_list() const;
	void check_leaders();
	void add_leader(const character *leader);
	void remove_leader(const character *leader);
	void clear_leaders();

	int get_leader_cost() const
	{
		int cost = 0;

		if (this->get_leaders().empty()) {
			cost = country_game_data::base_leader_cost / 2;
		} else {
			cost = country_game_data::base_leader_cost * static_cast<int>(this->get_leaders().size() + 1);
		}

		cost *= 100 + this->get_leader_cost_modifier();
		cost /= 100;

		return std::max(0, cost);
	}

	const character *get_next_leader() const
	{
		return this->next_leader;
	}

	void set_next_leader(const character *leader)
	{
		if (leader == this->get_next_leader()) {
			return;
		}

		this->next_leader = leader;
		emit next_leader_changed();
	}

	void choose_next_leader();
	const military_unit_type *get_next_leader_military_unit_type() const;

	bool has_civilian_character(const character *character) const;
	std::vector<const character *> get_civilian_characters() const;
	void check_civilian_characters();

	const commodity_map<int> &get_bids() const
	{
		return this->bids;
	}

	QVariantList get_bids_qvariant_list() const;

	Q_INVOKABLE int get_bid(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->bids.find(commodity);

		if (find_iterator != this->bids.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE void set_bid(const metternich::commodity *commodity, const int value);

	Q_INVOKABLE void change_bid(const metternich::commodity *commodity, const int change)
	{
		this->set_bid(commodity, this->get_bid(commodity) + change);
	}

	const commodity_map<int> &get_offers() const
	{
		return this->offers;
	}

	QVariantList get_offers_qvariant_list() const;

	Q_INVOKABLE int get_offer(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->offers.find(commodity);

		if (find_iterator != this->offers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE void set_offer(const metternich::commodity *commodity, const int value);

	Q_INVOKABLE void change_offer(const metternich::commodity *commodity, const int change)
	{
		this->set_offer(commodity, this->get_offer(commodity) + change);
	}

	void do_sale(const metternich::country *other_country, const commodity *commodity, const int sold_quantity, const bool state_purchase);

	const commodity_map<int> &get_commodity_needs() const
	{
		return this->commodity_needs;
	}

	int get_commodity_need(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->commodity_needs.find(commodity);

		if (find_iterator != this->commodity_needs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_need(const metternich::commodity *commodity, const int value)
	{
		if (value == this->get_commodity_need(commodity)) {
			return;
		}

		if (value == 0) {
			this->commodity_needs.erase(commodity);
		} else {
			this->commodity_needs[commodity] = value;
		}
	}

	void calculate_commodity_needs();

	void assign_trade_orders();

	void add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit);
	void remove_civilian_unit(civilian_unit *civilian_unit);

	void add_military_unit(qunique_ptr<military_unit> &&military_unit);
	void remove_military_unit(military_unit *military_unit);

	const std::set<std::string> &get_military_unit_names() const
	{
		return this->military_unit_names;
	}

	void add_army(qunique_ptr<army> &&army);
	void remove_army(army *army);

	void add_transporter(qunique_ptr<transporter> &&transporter);
	void remove_transporter(transporter *transporter);

	const military_unit_type *get_best_military_unit_category_type(const military_unit_category category, const culture *culture) const;
	const military_unit_type *get_best_military_unit_category_type(const military_unit_category category) const;

	const transporter_type *get_best_transporter_category_type(const transporter_category category, const culture *culture) const;
	const transporter_type *get_best_transporter_category_type(const transporter_category category) const;

	int get_deployment_limit() const
	{
		return this->deployment_limit;
	}

	void change_deployment_limit(const int change)
	{
		this->deployment_limit += change;
	}

	int get_land_damage_modifier() const
	{
		return this->land_damage_modifier;
	}

	void change_land_damage_modifier(const int change)
	{
		this->land_damage_modifier += change;
	}

	int get_land_recovery_modifier() const
	{
		return this->land_recovery_modifier;
	}

	void change_land_recovery_modifier(const int change)
	{
		this->land_recovery_modifier += change;
	}

	int get_land_morale_recovery_modifier() const
	{
		return this->land_morale_recovery_modifier;
	}

	void change_land_morale_recovery_modifier(const int change)
	{
		this->land_morale_recovery_modifier += change;
	}

	int get_land_discipline_modifier() const
	{
		return this->land_discipline_modifier;
	}

	void change_land_discipline_modifier(const int change)
	{
		this->land_discipline_modifier += change;
	}

	int get_naval_discipline_modifier() const
	{
		return this->naval_discipline_modifier;
	}

	void change_naval_discipline_modifier(const int change)
	{
		this->naval_discipline_modifier += change;
	}

	int get_air_discipline_modifier() const
	{
		return this->air_discipline_modifier;
	}

	void change_air_discipline_modifier(const int change)
	{
		this->air_discipline_modifier += change;
	}

	int get_entrenchment_bonus_modifier() const
	{
		return this->entrenchment_bonus_modifier;
	}

	void change_entrenchment_bonus_modifier(const int change)
	{
		this->entrenchment_bonus_modifier += change;
	}

	const centesimal_int &get_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat) const
	{
		const auto find_iterator = this->military_unit_type_stat_modifiers.find(type);

		if (find_iterator != this->military_unit_type_stat_modifiers.end()) {
			const auto sub_find_iterator = find_iterator->second.find(stat);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &value);

	void change_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &change)
	{
		this->set_military_unit_type_stat_modifier(type, stat, this->get_military_unit_type_stat_modifier(type, stat) + change);
	}

	const centesimal_int &get_transporter_type_stat_modifier(const transporter_type *type, const transporter_stat stat) const
	{
		const auto find_iterator = this->transporter_type_stat_modifiers.find(type);

		if (find_iterator != this->transporter_type_stat_modifiers.end()) {
			const auto sub_find_iterator = find_iterator->second.find(stat);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_transporter_type_stat_modifier(const transporter_type *type, const transporter_stat stat, const centesimal_int &value);

	void change_transporter_type_stat_modifier(const transporter_type *type, const transporter_stat stat, const centesimal_int &change)
	{
		this->set_transporter_type_stat_modifier(type, stat, this->get_transporter_type_stat_modifier(type, stat) + change);
	}

	int get_infantry_cost_modifier() const
	{
		return this->infantry_cost_modifier;
	}

	void change_infantry_cost_modifier(const int change)
	{
		this->infantry_cost_modifier += change;
	}

	int get_cavalry_cost_modifier() const
	{
		return this->cavalry_cost_modifier;
	}

	void change_cavalry_cost_modifier(const int change)
	{
		this->cavalry_cost_modifier += change;
	}

	int get_artillery_cost_modifier() const
	{
		return this->artillery_cost_modifier;
	}

	void change_artillery_cost_modifier(const int change)
	{
		this->artillery_cost_modifier += change;
	}

	int get_warship_cost_modifier() const
	{
		return this->warship_cost_modifier;
	}

	void change_warship_cost_modifier(const int change)
	{
		this->warship_cost_modifier += change;
	}

	int get_unit_upgrade_cost_modifier() const
	{
		return this->unit_upgrade_cost_modifier;
	}

	void change_unit_upgrade_cost_modifier(const int change)
	{
		this->unit_upgrade_cost_modifier += change;
	}

	int get_output_modifier() const
	{
		return this->output_modifier;
	}

	void set_output_modifier(const int value);

	void change_output_modifier(const int value)
	{
		this->set_output_modifier(this->get_output_modifier() + value);
	}

	int get_resource_output_modifier() const
	{
		return this->resource_output_modifier;
	}

	void set_resource_output_modifier(const int value);

	void change_resource_output_modifier(const int value)
	{
		this->set_resource_output_modifier(this->get_resource_output_modifier() + value);
	}

	int get_industrial_output_modifier() const
	{
		return this->industrial_output_modifier;
	}

	void set_industrial_output_modifier(const int value);

	void change_industrial_output_modifier(const int value)
	{
		this->set_industrial_output_modifier(this->get_industrial_output_modifier() + value);
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

	Q_INVOKABLE int get_commodity_output_modifier(metternich::commodity *commodity) const
	{
		const metternich::commodity *const_commodity = commodity;
		return this->get_commodity_output_modifier(const_commodity);
	}

	void set_commodity_output_modifier(const commodity *commodity, const int value);

	void change_commodity_output_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_output_modifier(commodity, this->get_commodity_output_modifier(commodity) + value);
	}

	const commodity_map<int> &get_capital_commodity_output_modifiers() const
	{
		return this->capital_commodity_output_modifiers;
	}

	int get_capital_commodity_output_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->capital_commodity_output_modifiers.find(commodity);

		if (find_iterator != this->capital_commodity_output_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_capital_commodity_output_modifier(const commodity *commodity, const int value);

	void change_capital_commodity_output_modifier(const commodity *commodity, const int value)
	{
		this->set_capital_commodity_output_modifier(commodity, this->get_capital_commodity_output_modifier(commodity) + value);
	}

	int get_throughput_modifier() const
	{
		return this->throughput_modifier;
	}

	void set_throughput_modifier(const int value);

	void change_throughput_modifier(const int value)
	{
		this->set_throughput_modifier(this->get_throughput_modifier() + value);
	}

	int get_commodity_throughput_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_throughput_modifiers.find(commodity);

		if (find_iterator != this->commodity_throughput_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_commodity_throughput_modifier(metternich::commodity *commodity) const
	{
		const metternich::commodity *const_commodity = commodity;
		return this->get_commodity_throughput_modifier(const_commodity);
	}

	void set_commodity_throughput_modifier(const commodity *commodity, const int value);

	void change_commodity_throughput_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_throughput_modifier(commodity, this->get_commodity_throughput_modifier(commodity) + value);
	}

	int get_improved_resource_commodity_bonus(const commodity *commodity, const resource *resource) const
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

	const improvement_map<commodity_map<centesimal_int>> &get_improvement_commodity_bonuses() const
	{
		return this->improvement_commodity_bonuses;
	}

	const commodity_map<centesimal_int> &get_improvement_commodity_bonuses(const improvement *improvement) const
	{
		const auto find_iterator = this->improvement_commodity_bonuses.find(improvement);

		if (find_iterator != this->improvement_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const commodity_map<centesimal_int> empty_map;
		return empty_map;
	}

	const centesimal_int &get_improvement_commodity_bonus(const commodity *commodity, const improvement *improvement) const
	{
		const auto find_iterator = this->improvement_commodity_bonuses.find(improvement);

		if (find_iterator != this->improvement_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void change_improvement_commodity_bonus(const improvement *improvement, const commodity *commodity, const centesimal_int &change);

	const building_type_map<commodity_map<int>> &get_building_commodity_bonuses() const
	{
		return this->building_commodity_bonuses;
	}

	const commodity_map<int> &get_building_commodity_bonuses(const building_type *building) const
	{
		const auto find_iterator = this->building_commodity_bonuses.find(building);

		if (find_iterator != this->building_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const commodity_map<int> empty_map;
		return empty_map;
	}

	int get_building_commodity_bonus(const commodity *commodity, const building_type *building) const
	{
		const auto find_iterator = this->building_commodity_bonuses.find(building);

		if (find_iterator != this->building_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void change_building_commodity_bonus(const building_type *building, const commodity *commodity, const int change);

	const profession_map<commodity_map<decimillesimal_int>> &get_profession_commodity_bonuses() const
	{
		return this->profession_commodity_bonuses;
	}

	const commodity_map<decimillesimal_int> &get_profession_commodity_bonuses(const profession *profession) const
	{
		const auto find_iterator = this->profession_commodity_bonuses.find(profession);

		if (find_iterator != this->profession_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const commodity_map<decimillesimal_int> empty_map;
		return empty_map;
	}

	const decimillesimal_int &get_profession_commodity_bonus(const profession *profession, const commodity *commodity) const
	{
		const auto find_iterator = this->profession_commodity_bonuses.find(profession);

		if (find_iterator != this->profession_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr decimillesimal_int zero;
		return zero;
	}

	void change_profession_commodity_bonus(const profession *profession, const commodity *commodity, const decimillesimal_int &change);

	int get_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold) const
	{
		const auto find_iterator = this->commodity_bonuses_for_tile_thresholds.find(commodity);

		if (find_iterator != this->commodity_bonuses_for_tile_thresholds.end()) {
			const auto sub_find_iterator = find_iterator->second.find(threshold);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value);

	void change_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
	{
		this->set_commodity_bonus_for_tile_threshold(commodity, threshold, this->get_commodity_bonus_for_tile_threshold(commodity, threshold) + value);
	}

	const commodity_map<centesimal_int> &get_commodity_bonuses_per_population() const
	{
		return this->commodity_bonuses_per_population;
	}

	const centesimal_int &get_commodity_bonus_per_population(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_bonuses_per_population.find(commodity);

		if (find_iterator != this->commodity_bonuses_per_population.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;

		return zero;
	}

	void change_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_capital_commodity_bonuses() const
	{
		return this->capital_commodity_bonuses;
	}

	const centesimal_int &get_capital_commodity_bonus(const commodity *commodity) const
	{
		const auto find_iterator = this->capital_commodity_bonuses.find(commodity);

		if (find_iterator != this->capital_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;

		return zero;
	}

	void change_capital_commodity_bonus(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_capital_commodity_bonuses_per_population() const
	{
		return this->capital_commodity_bonuses_per_population;
	}

	const centesimal_int &get_capital_commodity_bonus_per_population(const commodity *commodity) const
	{
		const auto find_iterator = this->capital_commodity_bonuses_per_population.find(commodity);

		if (find_iterator != this->capital_commodity_bonuses_per_population.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;

		return zero;
	}

	void change_capital_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change);

	Q_INVOKABLE int get_category_research_modifier(metternich::technology_category category) const
	{
		const auto find_iterator = this->category_research_modifiers.find(category);

		if (find_iterator != this->category_research_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_category_research_modifier(const technology_category category, const int value);

	void change_category_research_modifier(const technology_category category, const int value)
	{
		this->set_category_research_modifier(category, this->get_category_research_modifier(category) + value);
	}

	Q_INVOKABLE const centesimal_int &get_population_type_modifier_multiplier(const population_type *type) const
	{
		const auto find_iterator = this->population_type_modifier_multipliers.find(type);

		if (find_iterator != this->population_type_modifier_multipliers.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int one(1);
		return one;
	}

	void set_population_type_modifier_multiplier(const population_type *type, const centesimal_int &value);

	void change_population_type_modifier_multiplier(const population_type *type, const centesimal_int &change)
	{
		this->set_population_type_modifier_multiplier(type, this->get_population_type_modifier_multiplier(type) + change);
	}

	const centesimal_int &get_population_type_militancy_modifier(const population_type *type) const
	{
		const auto find_iterator = this->population_type_militancy_modifiers.find(type);

		if (find_iterator != this->population_type_militancy_modifiers.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_population_type_militancy_modifier(const population_type *type, const centesimal_int &value);

	void change_population_type_militancy_modifier(const population_type *type, const centesimal_int &change)
	{
		this->set_population_type_militancy_modifier(type, this->get_population_type_militancy_modifier(type) + change);
	}

	int get_law_cost_modifier() const
	{
		return this->law_cost_modifier;
	}

	void change_law_cost_modifier(const int change)
	{
		this->law_cost_modifier += change;
	}

	int get_tradition_cost_modifier() const
	{
		return this->tradition_cost_modifier;
	}

	void change_tradition_cost_modifier(const int change)
	{
		this->tradition_cost_modifier += change;
	}

	int get_advisor_cost_modifier() const
	{
		return this->advisor_cost_modifier;
	}

	void change_advisor_cost_modifier(const int change)
	{
		this->advisor_cost_modifier += change;
	}

	int get_leader_cost_modifier() const
	{
		return this->leader_cost_modifier;
	}

	void change_leader_cost_modifier(const int change)
	{
		this->leader_cost_modifier += change;
	}

	int get_building_cost_efficiency_modifier() const
	{
		return this->building_cost_efficiency_modifier;
	}

	void change_building_cost_efficiency_modifier(const int change)
	{
		this->building_cost_efficiency_modifier += change;
	}

	int get_wonder_cost_efficiency_modifier() const
	{
		return this->wonder_cost_efficiency_modifier;
	}

	void change_wonder_cost_efficiency_modifier(const int change)
	{
		this->wonder_cost_efficiency_modifier += change;
	}

	int get_diplomatic_penalty_for_expansion_modifier() const
	{
		return this->diplomatic_penalty_for_expansion_modifier;
	}

	void change_diplomatic_penalty_for_expansion_modifier(const int change)
	{
		this->diplomatic_penalty_for_expansion_modifier += change;
	}

	void decrement_scripted_modifiers();

	Q_INVOKABLE bool is_tile_explored(const QPoint &tile_pos) const;
	bool is_province_discovered(const province *province) const;

	bool is_province_explored(const province *province) const
	{
		//get whether the province has been fully explored
		return this->explored_provinces.contains(province);
	}

	bool is_region_discovered(const region *region) const;

	void explore_tile(const QPoint &tile_pos);
	void explore_province(const province *province);

	bool is_tile_prospected(const QPoint &tile_pos) const
	{
		return this->prospected_tiles.contains(tile_pos);
	}

	void prospect_tile(const QPoint &tile_pos);
	void reset_tile_prospection(const QPoint &tile_pos);

	const std::vector<const journal_entry *> &get_active_journal_entries() const
	{
		return this->active_journal_entries;
	}

	QVariantList get_active_journal_entries_qvariant_list() const;
	void add_active_journal_entry(const journal_entry *journal_entry);
	void remove_active_journal_entry(const journal_entry *journal_entry);

	const std::vector<const journal_entry *> &get_inactive_journal_entries() const
	{
		return this->inactive_journal_entries;
	}

	QVariantList get_inactive_journal_entries_qvariant_list() const;

	const std::vector<const journal_entry *> &get_finished_journal_entries() const
	{
		return this->finished_journal_entries;
	}

	QVariantList get_finished_journal_entries_qvariant_list() const;
	void check_journal_entries(const bool ignore_effects = false, const bool ignore_random_chance = false);
	bool check_potential_journal_entries();
	bool check_inactive_journal_entries();
	bool check_active_journal_entries(const read_only_context &ctx, const bool ignore_effects, const bool ignore_random_chance);

	int get_gain_technologies_known_by_others_count() const
	{
		return this->gain_technologies_known_by_others_count;
	}

	void set_gain_technologies_known_by_others_count(const int value);

	void change_gain_technologies_known_by_others_count(const int value)
	{
		this->set_gain_technologies_known_by_others_count(this->get_gain_technologies_known_by_others_count()  + value);
	}

	const building_class_map<int> &get_free_building_class_counts() const
	{
		return this->free_building_class_counts;
	}

	int get_free_building_class_count(const building_class *building_class) const
	{
		const auto find_iterator = this->free_building_class_counts.find(building_class);

		if (find_iterator != this->free_building_class_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_building_class_count(const building_class *building_class, const int value);

	void change_free_building_class_count(const building_class *building_class, const int value)
	{
		this->set_free_building_class_count(building_class, this->get_free_building_class_count(building_class) + value);
	}

	const promotion_map<int> &get_free_infantry_promotion_counts() const
	{
		return this->free_infantry_promotion_counts;
	}

	int get_free_infantry_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_infantry_promotion_counts.find(promotion);

		if (find_iterator != this->free_infantry_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_infantry_promotion_count(const promotion *promotion, const int value);

	void change_free_infantry_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_infantry_promotion_count(promotion, this->get_free_infantry_promotion_count(promotion) + value);
	}

	const promotion_map<int> &get_free_cavalry_promotion_counts() const
	{
		return this->free_cavalry_promotion_counts;
	}

	int get_free_cavalry_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_cavalry_promotion_counts.find(promotion);

		if (find_iterator != this->free_cavalry_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_cavalry_promotion_count(const promotion *promotion, const int value);

	void change_free_cavalry_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_cavalry_promotion_count(promotion, this->get_free_cavalry_promotion_count(promotion) + value);
	}

	const promotion_map<int> &get_free_artillery_promotion_counts() const
	{
		return this->free_artillery_promotion_counts;
	}

	int get_free_artillery_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_artillery_promotion_counts.find(promotion);

		if (find_iterator != this->free_artillery_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_artillery_promotion_count(const promotion *promotion, const int value);

	void change_free_artillery_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_artillery_promotion_count(promotion, this->get_free_artillery_promotion_count(promotion) + value);
	}

	const promotion_map<int> &get_free_warship_promotion_counts() const
	{
		return this->free_warship_promotion_counts;
	}

	int get_free_warship_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_warship_promotion_counts.find(promotion);

		if (find_iterator != this->free_warship_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_warship_promotion_count(const promotion *promotion, const int value);

	void change_free_warship_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_warship_promotion_count(promotion, this->get_free_warship_promotion_count(promotion) + value);
	}

	int get_free_consulate_count(const consulate *consulate) const
	{
		const auto find_iterator = this->free_consulate_counts.find(consulate);

		if (find_iterator != this->free_consulate_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_consulate_count(const consulate *consulate, const int value);

	void change_free_consulate_count(const consulate *consulate, const int value)
	{
		this->set_free_consulate_count(consulate, this->get_free_consulate_count(consulate) + value);
	}

	void calculate_tile_transport_levels();
	void clear_tile_transport_levels();

	bool has_flag(const flag *flag) const
	{
		return this->flags.contains(flag);
	}

	void set_flag(const flag *flag)
	{
		this->flags.insert(flag);
	}

	void clear_flag(const flag *flag)
	{
		this->flags.erase(flag);
	}

	int get_ai_building_desire_modifier(const building_type *building) const
	{
		const auto find_iterator = this->ai_building_desire_modifiers.find(building);

		if (find_iterator != this->ai_building_desire_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_ai_building_desire_modifier(const building_type *building, const int value)
	{
		if (value == this->get_ai_building_desire_modifier(building)) {
			return;
		}

		if (value == 0) {
			this->ai_building_desire_modifiers.erase(building);
		} else {
			this->ai_building_desire_modifiers[building] = value;
		}
	}

	void change_ai_building_desire_modifier(const building_type *building, const int value)
	{
		this->set_ai_building_desire_modifier(building, this->get_ai_building_desire_modifier(building) + value);
	}

	int get_ai_settlement_building_desire_modifier(const site *settlement, const building_type *building) const
	{
		const auto find_iterator = this->ai_settlement_building_desire_modifiers.find(settlement);

		if (find_iterator != this->ai_settlement_building_desire_modifiers.end()) {
			const auto sub_find_iterator = find_iterator->second.find(building);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void set_ai_settlement_building_desire_modifier(const site *settlement, const building_type *building, const int value)
	{
		if (value == this->get_ai_settlement_building_desire_modifier(settlement, building)) {
			return;
		}

		if (value == 0) {
			this->ai_settlement_building_desire_modifiers[settlement].erase(building);

			if (this->ai_settlement_building_desire_modifiers[settlement].empty()) {
				this->ai_settlement_building_desire_modifiers.erase(settlement);
			}
		} else {
			this->ai_settlement_building_desire_modifiers[settlement][building] = value;
		}
	}

	void change_ai_settlement_building_desire_modifier(const site *settlement, const building_type *building, const int value)
	{
		this->set_ai_settlement_building_desire_modifier(settlement, building, this->get_ai_settlement_building_desire_modifier(settlement, building) + value);
	}

signals:
	void tier_changed();
	void title_name_changed();
	void ruler_title_name_changed();
	void religion_changed();
	void overlord_changed();
	void type_name_changed();
	void subject_type_changed();
	void diplomacy_states_changed();
	void offered_diplomacy_states_changed();
	void consulates_changed();
	void provinces_changed();
	void capital_changed();
	void diplomatic_map_image_changed();
	void score_changed();
	void score_rank_changed();
	void rank_changed();
	void population_units_changed();
	void population_growth_changed();
	void health_changed();
	void settlement_building_counts_changed();
	void wealth_changed();
	void wealth_income_changed();
	void credit_limit_changed();
	void inflation_changed();
	void available_commodities_changed();
	void tradeable_commodities_changed();
	void stored_commodities_changed();
	void storage_capacity_changed();
	void commodity_inputs_changed();
	void transportable_commodity_outputs_changed();
	void transported_commodity_outputs_changed();
	void commodity_outputs_changed();
	void everyday_wealth_consumption_changed();
	void everyday_consumption_changed();
	void luxury_consumption_changed();
	void land_transport_capacity_changed();
	void sea_transport_capacity_changed();
	void technologies_changed();
	void current_research_changed();
	void technology_researched(const technology *technology);
	void government_type_changed();
	void laws_changed();
	void policy_values_changed();
	void traditions_changed();
	void next_tradition_changed();
	void tradition_adopted(const tradition *tradition);
	void next_belief_changed();
	void belief_adopted(const tradition *belief);
	void ruler_changed();
	void advisors_changed();
	void next_advisor_changed();
	void advisor_recruited(const character *advisor);
	void leaders_changed();
	void next_leader_changed();
	void leader_recruited(const character *leader);
	void bids_changed();
	void offers_changed();
	void output_modifier_changed();
	void resource_output_modifier_changed();
	void industrial_output_modifier_changed();
	void throughput_modifier_changed();
	void prospected_tiles_changed();
	void journal_entries_changed();
	void journal_entry_completed(const journal_entry *journal_entry);

private:
	metternich::country *country = nullptr;
	country_tier tier{};
	const metternich::religion *religion = nullptr;
	const metternich::country *overlord = nullptr;
	std::vector<const province *> provinces;
	const site *capital = nullptr;
	int settlement_count = 0; //only includes built settlements
	std::vector<const province *> border_provinces;
	int coastal_province_count = 0;
	QRect territory_rect;
	std::vector<QRect> contiguous_territory_rects;
	QRect main_contiguous_territory_rect;
	QRect text_rect;
	QPoint territory_rect_center = QPoint(-1, -1);
	std::vector<QPoint> border_tiles;
	resource_map<int> resource_counts;
	resource_map<int> vassal_resource_counts;
	terrain_type_map<int> tile_terrain_counts;
	const metternich::subject_type *subject_type = nullptr;
	country_set known_countries;
	country_map<diplomacy_state> diplomacy_states;
	std::map<diplomacy_state, int> diplomacy_state_counts;
	country_map<diplomacy_state> offered_diplomacy_states;
	country_map<const consulate *> consulates;
	country_map<int> base_opinions;
	country_map<opinion_modifier_map<int>> opinion_modifiers;
	QImage diplomatic_map_image;
	QImage selected_diplomatic_map_image;
	std::map<diplomatic_map_mode, QImage> diplomatic_map_mode_images;
	std::map<diplomacy_state, QImage> diplomacy_state_diplomatic_map_images;
	QRect diplomatic_map_image_rect;
	int score = 0;
	const country_rank *rank = nullptr;
	int score_rank = 0;
	int economic_score = 0;
	int military_score = 0;
	std::vector<population_unit *> population_units;
	qunique_ptr<metternich::population> population;
	int population_growth = 0; //population growth counter
	centesimal_int health;
	int food_consumption = 0;
	std::vector<qunique_ptr<country_building_slot>> building_slots;
	building_slot_type_map<country_building_slot *> building_slot_map;
	building_type_map<int> settlement_building_counts;
	int wealth = 0;
	int wealth_income = 0;
	int credit_limit = 0;
	centesimal_int inflation;
	centesimal_int inflation_change;
	commodity_set available_commodities;
	commodity_set tradeable_commodities;
	commodity_map<int> stored_commodities;
	int storage_capacity = 0;
	commodity_map<int> commodity_inputs;
	commodity_map<centesimal_int> transportable_commodity_outputs;
	commodity_map<int> transported_commodity_outputs;
	commodity_map<centesimal_int> commodity_outputs;
	int everyday_wealth_consumption = 0;
	commodity_map<centesimal_int> everyday_consumption;
	commodity_map<centesimal_int> luxury_consumption;
	commodity_map<centesimal_int> commodity_demands;
	int land_transport_capacity = 0;
	int sea_transport_capacity = 0;
	technology_set technologies;
	const technology *current_research = nullptr;
	int free_technology_count = 0;
	const metternich::government_type *government_type = nullptr;
	law_group_map<const law *> laws;
	policy_map<int> policy_values;
	tradition_set traditions;
	const tradition *next_tradition = nullptr;
	const tradition *next_belief = nullptr;
	const character *ruler = nullptr;
	std::vector<const character *> advisors;
	const character *next_advisor = nullptr;
	std::vector<const character *> leaders;
	const character *next_leader = nullptr;
	commodity_map<int> bids;
	commodity_map<int> offers;
	commodity_map<int> commodity_needs;
	std::vector<qunique_ptr<civilian_unit>> civilian_units;
	std::vector<qunique_ptr<military_unit>> military_units;
	std::set<std::string> military_unit_names;
	std::vector<qunique_ptr<army>> armies;
	std::vector<qunique_ptr<transporter>> transporters;
	int deployment_limit = country_game_data::base_deployment_limit;
	int land_damage_modifier = 0;
	int land_recovery_modifier = 0; //the speed at which land units recover strength
	int land_morale_recovery_modifier = 0;
	int land_discipline_modifier = 0;
	int naval_discipline_modifier = 0;
	int air_discipline_modifier = 0;
	int entrenchment_bonus_modifier = 0;
	military_unit_type_map<std::map<military_unit_stat, centesimal_int>> military_unit_type_stat_modifiers;
	transporter_type_map<std::map<transporter_stat, centesimal_int>> transporter_type_stat_modifiers;
	int infantry_cost_modifier = 0;
	int cavalry_cost_modifier = 0;
	int artillery_cost_modifier = 0;
	int warship_cost_modifier = 0;
	int unit_upgrade_cost_modifier = 0;
	int output_modifier = 0;
	int resource_output_modifier = 0;
	int industrial_output_modifier = 0;
	commodity_map<int> commodity_output_modifiers;
	commodity_map<int> capital_commodity_output_modifiers;
	int throughput_modifier = 0;
	commodity_map<int> commodity_throughput_modifiers;
	resource_map<commodity_map<int>> improved_resource_commodity_bonuses;
	improvement_map<commodity_map<centesimal_int>> improvement_commodity_bonuses;
	building_type_map<commodity_map<int>> building_commodity_bonuses;
	profession_map<commodity_map<decimillesimal_int>> profession_commodity_bonuses;
	commodity_map<std::map<int, int>> commodity_bonuses_for_tile_thresholds;
	commodity_map<centesimal_int> commodity_bonuses_per_population;
	commodity_map<centesimal_int> capital_commodity_bonuses;
	commodity_map<centesimal_int> capital_commodity_bonuses_per_population;
	std::map<technology_category, int> category_research_modifiers;
	population_type_map<centesimal_int> population_type_modifier_multipliers;
	population_type_map<centesimal_int> population_type_militancy_modifiers;
	int law_cost_modifier = 0;
	int tradition_cost_modifier = 0;
	int advisor_cost_modifier = 0;
	int leader_cost_modifier = 0;
	int building_cost_efficiency_modifier = 0;
	int wonder_cost_efficiency_modifier = 0;
	int diplomatic_penalty_for_expansion_modifier = 0;
	province_set explored_provinces;
	point_set explored_tiles; //used for tiles in partially-explored provinces
	point_set prospected_tiles;
	std::vector<const journal_entry *> active_journal_entries;
	std::vector<const journal_entry *> inactive_journal_entries;
	std::vector<const journal_entry *> finished_journal_entries;
	int gain_technologies_known_by_others_count = 0;
	building_class_map<int> free_building_class_counts;
	promotion_map<int> free_infantry_promotion_counts;
	promotion_map<int> free_cavalry_promotion_counts;
	promotion_map<int> free_artillery_promotion_counts;
	promotion_map<int> free_warship_promotion_counts;
	consulate_map<int> free_consulate_counts;
	std::set<const flag *> flags;
	building_type_map<int> ai_building_desire_modifiers;
	site_map<building_type_map<int>> ai_settlement_building_desire_modifiers;
};

}
