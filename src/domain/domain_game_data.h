#pragma once

#include "database/data_entry_container.h"
#include "domain/consulate_container.h"
#include "domain/domain_container.h"
#include "economy/commodity_container.h"
#include "infrastructure/building_class_container.h"
#include "infrastructure/building_type_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "map/province_container.h"
#include "map/site_container.h"
#include "map/terrain_type_container.h"
#include "population/population_type_container.h"
#include "script/opinion_modifier_container.h"
#include "script/scripted_modifier_container.h"
#include "unit/transporter_type_container.h"
#include "util/centesimal_int.h"
#include "util/point_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("domain/country_economy.h")
Q_MOC_INCLUDE("domain/country_government.h")
Q_MOC_INCLUDE("domain/country_military.h")
Q_MOC_INCLUDE("domain/country_technology.h")
Q_MOC_INCLUDE("domain/country_tier.h")
Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("domain/government_type.h")
Q_MOC_INCLUDE("domain/journal_entry.h")
Q_MOC_INCLUDE("domain/subject_type.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("population/population.h")
Q_MOC_INCLUDE("religion/religion.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")
Q_MOC_INCLUDE("unit/transporter_type.h")

namespace metternich {

class building_type;
class character;
class civilian_unit;
class consulate;
class country_ai;
class country_economy;
class country_government;
class country_military;
class country_rank;
class country_technology;
class culture;
class domain;
class event;
class flag;
class government_type;
class idea;
class idea_slot;
class journal_entry;
class opinion_modifier;
class phenotype;
class population;
class population_class;
class population_type;
class population_unit;
class portrait;
class province;
class region;
class religion;
class scripted_country_modifier;
class site;
class subject_type;
class transporter;
class transporter_type;
class wonder;
enum class country_tier;
enum class diplomacy_state;
enum class diplomatic_map_mode;
enum class event_trigger;
enum class idea_type;
enum class transporter_category;
enum class transporter_stat;
struct read_only_context;

template <typename scope_type>
class modifier;

class domain_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_economy* economy READ get_economy CONSTANT)
	Q_PROPERTY(metternich::country_government* government READ get_government CONSTANT)
	Q_PROPERTY(metternich::country_military* military READ get_military CONSTANT)
	Q_PROPERTY(metternich::country_technology* technology READ get_technology CONSTANT)
	Q_PROPERTY(metternich::country_tier tier READ get_tier NOTIFY tier_changed)
	Q_PROPERTY(QString name READ get_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString title_name READ get_title_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(const metternich::religion* religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(const metternich::domain* overlord READ get_overlord NOTIFY overlord_changed)
	Q_PROPERTY(QString type_name READ get_type_name_qstring NOTIFY type_name_changed)
	Q_PROPERTY(const metternich::subject_type* subject_type READ get_subject_type NOTIFY subject_type_changed)
	Q_PROPERTY(const metternich::government_type *government_type READ get_government_type NOTIFY government_type_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList sites READ get_sites_qvariant_list NOTIFY sites_changed)
	Q_PROPERTY(const metternich::site* capital READ get_capital NOTIFY capital_changed)
	Q_PROPERTY(bool coastal READ is_coastal NOTIFY provinces_changed)
	Q_PROPERTY(bool anarchy READ is_under_anarchy NOTIFY provinces_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QPoint territory_rect_center READ get_territory_rect_center NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList contiguous_territory_rects READ get_contiguous_territory_rects_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QRect main_contiguous_territory_rect READ get_main_contiguous_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QRect text_rect READ get_text_rect NOTIFY provinces_changed)
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
	Q_PROPERTY(int housing READ get_housing_int NOTIFY housing_changed)
	Q_PROPERTY(QColor diplomatic_map_color READ get_diplomatic_map_color NOTIFY overlord_changed)
	Q_PROPERTY(QVariantList ideas READ get_ideas_qvariant_list NOTIFY ideas_changed)
	Q_PROPERTY(QVariantList appointed_ideas READ get_appointed_ideas_qvariant_list NOTIFY appointed_ideas_changed)
	Q_PROPERTY(QVariantList available_deity_slots READ get_available_deity_slots_qvariant_list NOTIFY available_idea_slots_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(QVariantList characters READ get_characters_qvariant_list NOTIFY characters_changed)
	Q_PROPERTY(QVariantList transporters READ get_transporters_qvariant_list NOTIFY transporters_changed)
	Q_PROPERTY(QVariantList active_journal_entries READ get_active_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(QVariantList inactive_journal_entries READ get_inactive_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(QVariantList finished_journal_entries READ get_finished_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(int min_income READ get_min_income NOTIFY income_changed)
	Q_PROPERTY(int max_income READ get_max_income NOTIFY income_changed)
	Q_PROPERTY(int maintenance_cost READ get_maintenance_cost NOTIFY maintenance_cost_changed)

public:
	static constexpr int first_deity_cost = 10;
	static constexpr int base_deity_cost = 200;
	static constexpr int deity_cost_increment = 100;
	static constexpr int vassal_tax_rate = 50;

	explicit domain_game_data(metternich::domain *domain);
	~domain_game_data();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	void do_turn();
	void collect_wealth();
	void pay_maintenance();
	void do_civilian_unit_recruitment();
	void do_transporter_recruitment();
	void do_population_growth();
	void do_food_consumption(const int food_consumption);
	void do_starvation();
	void do_cultural_change();
	void do_events();

	country_economy *get_economy() const
	{
		return this->economy.get();
	}

	country_government *get_government() const
	{
		return this->government.get();
	}

	country_military *get_military() const
	{
		return this->military.get();
	}

	country_technology *get_technology() const
	{
		return this->technology.get();
	}

	bool is_ai() const;
	country_ai *get_ai() const;

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

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	void set_religion(const metternich::religion *religion);

	const metternich::domain *get_overlord() const
	{
		return this->overlord;
	}

	void set_overlord(const metternich::domain *overlord);

	bool is_vassal_of(const metternich::domain *domain) const;
	bool is_any_vassal_of(const metternich::domain *domain) const;

	Q_INVOKABLE bool is_any_vassal_of(metternich::domain *domain)
	{
		const metternich::domain *domain_const = domain;
		return this->is_any_vassal_of(domain_const);
	}

	bool is_overlord_of(const metternich::domain *domain) const;
	bool is_any_overlord_of(const metternich::domain *domain) const;

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

	const metternich::government_type *get_government_type() const
	{
		return this->government_type;
	}

	void set_government_type(const metternich::government_type *government_type);
	bool can_have_government_type(const metternich::government_type *government_type) const;
	void check_government_type();

	bool is_tribal() const;
	bool is_clade() const;

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

	const std::vector<const site *> &get_sites() const
	{
		return this->sites;
	}

	QVariantList get_sites_qvariant_list() const;
	void add_site(const site *site);
	void remove_site(const site *site);
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
		return !this->get_provinces().empty() && !this->get_sites().empty();
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

	const domain_set &get_known_countries() const
	{
		return this->known_countries;
	}

	bool is_country_known(const metternich::domain *other_domain) const
	{
		return this->get_known_countries().contains(other_domain);
	}

	void add_known_country(const metternich::domain *other_domain);

	void remove_known_country(const metternich::domain *other_domain)
	{
		this->known_countries.erase(other_domain);
	}

	diplomacy_state get_diplomacy_state(const metternich::domain *other_domain) const;
	void set_diplomacy_state(const metternich::domain *other_domain, const diplomacy_state state);

	const std::map<diplomacy_state, int> &get_diplomacy_state_counts() const
	{
		return this->diplomacy_state_counts;
	}

	void change_diplomacy_state_count(const diplomacy_state state, const int change);
	Q_INVOKABLE QString get_diplomacy_state_diplomatic_map_suffix(metternich::domain *other_domain) const;

	bool at_war() const;

	bool can_attack(const metternich::domain *other_domain) const;

	std::optional<diplomacy_state> get_offered_diplomacy_state(const metternich::domain *other_domain) const;

	Q_INVOKABLE int get_offered_diplomacy_state_int(metternich::domain *other_domain) const
	{
		const std::optional<diplomacy_state> state = this->get_offered_diplomacy_state(other_domain);

		if (!state.has_value()) {
			return -1;
		}

		return static_cast<int>(state.value());
	}

	void set_offered_diplomacy_state(const metternich::domain *other_domain, const std::optional<diplomacy_state> &state);

	Q_INVOKABLE void set_offered_diplomacy_state_int(metternich::domain *other_domain, const int state)
	{
		if (state == -1) {
			this->set_offered_diplomacy_state(other_domain, std::nullopt);
		} else {
			this->set_offered_diplomacy_state(other_domain, static_cast<diplomacy_state>(state));
		}
	}

	QVariantList get_consulates_qvariant_list() const;

	const consulate *get_consulate(const metternich::domain *other_domain) const
	{
		const auto find_iterator = this->consulates.find(other_domain);

		if (find_iterator != this->consulates.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_consulate(const metternich::domain *other_domain, const consulate *consulate);

	int get_opinion_of(const metternich::domain *other) const;

	int get_base_opinion(const metternich::domain *other) const
	{
		const auto find_iterator = this->base_opinions.find(other);
		if (find_iterator != this->base_opinions.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_base_opinion(const metternich::domain *other, const int opinion);

	void change_base_opinion(const metternich::domain *other, const int change)
	{
		this->set_base_opinion(other, this->get_base_opinion(other) + change);
	}

	const opinion_modifier_map<int> &get_opinion_modifiers_for(const metternich::domain *other) const
	{
		static const opinion_modifier_map<int> empty_map;

		const auto find_iterator = this->opinion_modifiers.find(other);
		if (find_iterator != this->opinion_modifiers.end()) {
			return find_iterator->second;
		}

		return empty_map;
	}

	void add_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier, const int duration);
	void remove_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier);

	std::vector<const metternich::domain *> get_vassals() const;
	QVariantList get_vassals_qvariant_list() const;
	QVariantList get_subject_type_counts_qvariant_list() const;

	std::vector<const metternich::domain *> get_neighbor_countries() const;

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

	std::vector<const phenotype *> get_weighted_phenotypes() const;

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
	void decrease_population(const bool change_population_growth);
	population_unit *choose_starvation_population_unit();

	Q_INVOKABLE const icon *get_population_type_small_icon(const metternich::population_type *type) const;

	const centesimal_int &get_housing() const
	{
		return this->housing;
	}

	int get_housing_int() const
	{
		return this->get_housing().to_int();
	}

	void change_housing(const centesimal_int &change)
	{
		if (change == 0) {
			return;
		}

		this->housing += change;

		emit housing_changed();
	}

	centesimal_int get_available_housing() const
	{
		return this->get_housing() - this->get_population_unit_count();
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
	int get_available_food() const;

	bool has_building(const building_type *building) const;
	bool has_building_or_better(const building_type *building) const;

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

	bool can_declare_war_on(const metternich::domain *other_domain) const;

	const std::map<idea_type, data_entry_map<idea_slot, const idea *>> &get_ideas() const
	{
		return this->ideas;
	}

	const data_entry_map<idea_slot, const idea *> &get_ideas(const idea_type idea_type) const
	{
		const auto find_iterator = this->ideas.find(idea_type);

		if (find_iterator != this->ideas.end()) {
			return find_iterator->second;
		}

		static const data_entry_map<idea_slot, const idea *> empty_ideas;
		return empty_ideas;
	}

	QVariantList get_ideas_qvariant_list() const;
	Q_INVOKABLE const metternich::idea *get_idea(const metternich::idea_slot *slot) const;
	void set_idea(const idea_slot *slot, const idea *idea);

	const std::map<idea_type, data_entry_map<idea_slot, const idea *>> &get_appointed_ideas() const
	{
		return this->appointed_ideas;
	}

	const data_entry_map<idea_slot, const idea *> &get_appointed_ideas(const idea_type idea_type) const
	{
		const auto find_iterator = this->appointed_ideas.find(idea_type);

		if (find_iterator != this->appointed_ideas.end()) {
			return find_iterator->second;
		}

		static const data_entry_map<idea_slot, const idea *> empty_ideas;
		return empty_ideas;
	}

	QVariantList get_appointed_ideas_qvariant_list() const;
	Q_INVOKABLE const metternich::idea *get_appointed_idea(const metternich::idea_slot *slot) const;
	Q_INVOKABLE void set_appointed_idea(const metternich::idea_slot *slot, const metternich::idea *idea);

	void check_idea(const idea_slot *slot);
	void check_ideas();
	std::vector<const idea *> get_appointable_ideas(const idea_slot *slot) const;
	Q_INVOKABLE QVariantList get_appointable_ideas_qvariant_list(const metternich::idea_slot *slot) const;
	const idea *get_best_idea(const idea_slot *slot);
	bool can_have_idea(const idea_slot *slot, const idea *idea) const;
	bool can_gain_idea(const idea_slot *slot, const idea *idea) const;
	Q_INVOKABLE bool can_appoint_idea(const metternich::idea_slot *slot, const metternich::idea *idea) const;

	std::vector<const idea_slot *> get_available_idea_slots(const idea_type idea_type) const;
	QVariantList get_available_deity_slots_qvariant_list() const;

	int get_deity_cost() const;
	commodity_map<int> get_idea_commodity_costs(const idea *idea) const;
	Q_INVOKABLE QVariantList get_idea_commodity_costs_qvariant_list(const metternich::idea *idea) const;

	const scripted_country_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_country_modifier *modifier) const;
	void add_scripted_modifier(const scripted_country_modifier *modifier, const int duration);
	void remove_scripted_modifier(const scripted_country_modifier *modifier);
	void decrement_scripted_modifiers();

	void apply_modifier(const modifier<const metternich::domain> *modifier, const int multiplier = 1);

	void remove_modifier(const modifier<const metternich::domain> *modifier)
	{
		this->apply_modifier(modifier, -1);
	}

	std::vector<const character *> get_characters() const;
	QVariantList get_characters_qvariant_list() const;
	void add_character(const character *character);
	void remove_character(const character *character);
	void check_characters();

	void generate_ruler();

	const std::vector<qunique_ptr<civilian_unit>> &get_civilian_units() const
	{
		return this->civilian_units;
	}

	bool create_civilian_unit(const civilian_unit_type *civilian_unit_type, const site *deployment_site, const phenotype *phenotype);
	void add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit);
	void remove_civilian_unit(civilian_unit *civilian_unit);

	Q_INVOKABLE bool can_gain_civilian_unit(const metternich::civilian_unit_type *civilian_unit_type) const;

	Q_INVOKABLE int get_civilian_unit_recruitment_count(const metternich::civilian_unit_type *civilian_unit_type) const
	{
		const auto find_iterator = this->civilian_unit_recruitment_counts.find(civilian_unit_type);

		if (find_iterator != this->civilian_unit_recruitment_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_civilian_unit_recruitment_count(const civilian_unit_type *civilian_unit_type, const int change, const bool change_input_storage = true);
	Q_INVOKABLE bool can_increase_civilian_unit_recruitment(const metternich::civilian_unit_type *civilian_unit_type) const;
	Q_INVOKABLE void increase_civilian_unit_recruitment(const metternich::civilian_unit_type *civilian_unit_type);
	Q_INVOKABLE bool can_decrease_civilian_unit_recruitment(const metternich::civilian_unit_type *civilian_unit_type) const;
	Q_INVOKABLE void decrease_civilian_unit_recruitment(const metternich::civilian_unit_type *civilian_unit_type, const bool restore_inputs);

	QVariantList get_transporters_qvariant_list() const;
	bool create_transporter(const transporter_type *transporter_type, const phenotype *phenotype);
	void add_transporter(qunique_ptr<transporter> &&transporter);
	void remove_transporter(transporter *transporter);

	Q_INVOKABLE int get_transporter_recruitment_count(const metternich::transporter_type *transporter_type) const
	{
		const auto find_iterator = this->transporter_recruitment_counts.find(transporter_type);

		if (find_iterator != this->transporter_recruitment_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_transporter_recruitment_count(const transporter_type *transporter_type, const int change, const bool change_input_storage = true);
	Q_INVOKABLE bool can_increase_transporter_recruitment(const metternich::transporter_type *transporter_type) const;
	Q_INVOKABLE void increase_transporter_recruitment(const metternich::transporter_type *transporter_type);
	Q_INVOKABLE bool can_decrease_transporter_recruitment(const metternich::transporter_type *transporter_type) const;
	Q_INVOKABLE void decrease_transporter_recruitment(const metternich::transporter_type *transporter_type, const bool restore_inputs);

	int get_transporter_type_cost_modifier(const transporter_type *transporter_type) const;
	Q_INVOKABLE int get_transporter_type_wealth_cost(const metternich::transporter_type *transporter_type, const int quantity) const;
	commodity_map<int> get_transporter_type_commodity_costs(const transporter_type *transporter_type, const int quantity) const;
	Q_INVOKABLE QVariantList get_transporter_type_commodity_costs_qvariant_list(const metternich::transporter_type *transporter_type, const int quantity) const;

	const transporter_type *get_best_transporter_category_type(const transporter_category category, const culture *culture) const;
	Q_INVOKABLE const metternich::transporter_type *get_best_transporter_category_type(const metternich::transporter_category category) const;

	const std::map<std::string, int> &get_unit_name_counts() const
	{
		return this->unit_name_counts;
	}

	void add_unit_name(const std::string &name)
	{
		++this->unit_name_counts[name];
	}

	void remove_unit_name(const std::string &name)
	{
		const int count = (this->unit_name_counts[name] -= 1);
		if (count == 0) {
			this->unit_name_counts.erase(name);
		}
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

	Q_INVOKABLE const centesimal_int &get_population_type_modifier_multiplier(const population_type *type) const
	{
		const auto find_iterator = this->population_type_modifier_multipliers.find(type);

		if (find_iterator != this->population_type_modifier_multipliers.end()) {
			return find_iterator->second;
		}

		static const centesimal_int one(1);
		return one;
	}

	void set_population_type_modifier_multiplier(const population_type *type, const centesimal_int &value);

	void change_population_type_modifier_multiplier(const population_type *type, const centesimal_int &change)
	{
		this->set_population_type_modifier_multiplier(type, this->get_population_type_modifier_multiplier(type) + change);
	}

	int get_building_cost_efficiency_modifier() const
	{
		return this->building_cost_efficiency_modifier;
	}

	void change_building_cost_efficiency_modifier(const int change)
	{
		this->building_cost_efficiency_modifier += change;
	}

	int get_building_class_cost_efficiency_modifier(const building_class *building_class) const
	{
		const auto find_iterator = this->building_class_cost_efficiency_modifiers.find(building_class);

		if (find_iterator != this->building_class_cost_efficiency_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_building_class_cost_efficiency_modifier(const building_class *building_class, const int value);

	void change_building_class_cost_efficiency_modifier(const building_class *building_class, const int value)
	{
		this->set_building_class_cost_efficiency_modifier(building_class, this->get_building_class_cost_efficiency_modifier(building_class) + value);
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

	const point_set &get_explored_tiles() const
	{
		return this->explored_tiles;
	}

	Q_INVOKABLE bool is_tile_explored(const QPoint &tile_pos) const;
	bool is_province_discovered(const province *province) const;

	const province_set &get_explored_provinces() const
	{
		return this->explored_provinces;
	}

	bool is_province_explored(const province *province) const
	{
		//get whether the province has been fully explored
		return this->explored_provinces.contains(province);
	}

	bool is_region_discovered(const region *region) const;

	void explore_tile(const QPoint &tile_pos);
	void explore_province(const province *province);

	const point_set &get_prospected_tiles() const
	{
		return this->prospected_tiles;
	}

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

	int get_min_income() const;
	int get_max_income() const;
	int get_domain_maintenance_cost() const;
	int get_maintenance_cost() const;

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

signals:
	void tier_changed();
	void title_name_changed();
	void religion_changed();
	void overlord_changed();
	void type_name_changed();
	void subject_type_changed();
	void government_type_changed();
	void diplomacy_states_changed();
	void offered_diplomacy_states_changed();
	void consulates_changed();
	void provinces_changed();
	void sites_changed();
	void capital_changed();
	void diplomatic_map_image_changed();
	void score_changed();
	void score_rank_changed();
	void rank_changed();
	void population_units_changed();
	void population_growth_changed();
	void housing_changed();
	void population_type_inputs_changed();
	void population_type_outputs_changed();
	void settlement_building_counts_changed();
	void ideas_changed();
	void appointed_ideas_changed();
	void available_idea_slots_changed();
	void scripted_modifiers_changed();
	void characters_changed();
	void transporters_changed();
	void prospected_tiles_changed();
	void journal_entries_changed();
	void journal_entry_completed(const journal_entry *journal_entry);
	void income_changed();
	void maintenance_cost_changed();

private:
	metternich::domain *domain = nullptr;
	country_tier tier{};
	const metternich::religion *religion = nullptr;
	const metternich::domain *overlord = nullptr;
	const metternich::government_type *government_type = nullptr;
	std::vector<const province *> provinces;
	std::vector<const site *> sites;
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
	terrain_type_map<int> tile_terrain_counts;
	const metternich::subject_type *subject_type = nullptr;
	domain_set known_countries;
	domain_map<diplomacy_state> diplomacy_states;
	std::map<diplomacy_state, int> diplomacy_state_counts;
	domain_map<diplomacy_state> offered_diplomacy_states;
	domain_map<const consulate *> consulates;
	domain_map<int> base_opinions;
	domain_map<opinion_modifier_map<int>> opinion_modifiers;
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
	centesimal_int housing;
	int food_consumption = 0;
	building_type_map<int> settlement_building_counts;
	std::map<idea_type, data_entry_map<idea_slot, const idea *>> ideas;
	std::map<idea_type, data_entry_map<idea_slot, const idea *>> appointed_ideas;
	scripted_country_modifier_map<int> scripted_modifiers;
	std::vector<const character *> characters;
	std::vector<qunique_ptr<civilian_unit>> civilian_units;
	data_entry_map<civilian_unit_type, int> civilian_unit_recruitment_counts;
	std::vector<qunique_ptr<transporter>> transporters;
	transporter_type_map<int> transporter_recruitment_counts;
	std::map<std::string, int> unit_name_counts;
	transporter_type_map<std::map<transporter_stat, centesimal_int>> transporter_type_stat_modifiers;
	population_type_map<centesimal_int> population_type_modifier_multipliers;
	int building_cost_efficiency_modifier = 0;
	building_class_map<int> building_class_cost_efficiency_modifiers;
	int wonder_cost_efficiency_modifier = 0;
	int diplomatic_penalty_for_expansion_modifier = 0;
	province_set explored_provinces;
	point_set explored_tiles; //used for tiles in partially-explored provinces
	point_set prospected_tiles;
	std::vector<const journal_entry *> active_journal_entries;
	std::vector<const journal_entry *> inactive_journal_entries;
	std::vector<const journal_entry *> finished_journal_entries;
	building_class_map<int> free_building_class_counts;
	consulate_map<int> free_consulate_counts;
	std::set<const flag *> flags;
	qunique_ptr<country_economy> economy;
	qunique_ptr<country_government> government;
	qunique_ptr<country_military> military;
	qunique_ptr<country_technology> technology;
};

}
