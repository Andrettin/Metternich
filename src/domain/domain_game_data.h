#pragma once

#include "database/data_entry_container.h"
#include "economy/commodity_container.h"
#include "infrastructure/building_class_container.h"
#include "infrastructure/building_type_container.h"
#include "map/province_container.h"
#include "map/terrain_type_container.h"
#include "population/population_type_container.h"
#include "script/scripted_modifier_container.h"
#include "species/phenotype_container.h"
#include "unit/transporter_type_container.h"
#include "util/centesimal_int.h"
#include "util/decimillesimal_int.h"
#include "util/point_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("culture/culture.h")
Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("domain/domain_diplomacy.h")
Q_MOC_INCLUDE("domain/domain_economy.h")
Q_MOC_INCLUDE("domain/domain_government.h")
Q_MOC_INCLUDE("domain/domain_military.h")
Q_MOC_INCLUDE("domain/domain_technology.h")
Q_MOC_INCLUDE("domain/domain_tier.h")
Q_MOC_INCLUDE("domain/government_type.h")
Q_MOC_INCLUDE("domain/journal_entry.h")
Q_MOC_INCLUDE("infrastructure/building_type.h")
Q_MOC_INCLUDE("infrastructure/pathway.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("population/population.h")
Q_MOC_INCLUDE("religion/religion.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")
Q_MOC_INCLUDE("unit/transporter_type.h")

namespace archimedes {
	class gsml_data;
	class gsml_property;
	enum class gender;
}

namespace metternich {

class building_item_slot;
class building_type;
class character;
class civilian_unit;
class consulate;
class culture;
class domain;
class domain_ai;
class domain_attribute;
class domain_diplomacy;
class domain_economy;
class domain_government;
class domain_military;
class domain_rank;
class domain_technology;
class dynasty;
class event;
class flag;
class government_type;
class idea;
class idea_slot;
class journal_entry;
class monster_type;
class opinion_modifier;
class pathway;
class phenotype;
class population;
class population_class;
class population_type;
class population_unit;
class portrait;
class province;
class region;
class religion;
class scripted_domain_modifier;
class site;
class site_attribute;
class transporter;
class transporter_type;
class wonder;
enum class domain_tier;
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

	Q_PROPERTY(metternich::domain_diplomacy* diplomacy READ get_diplomacy CONSTANT)
	Q_PROPERTY(metternich::domain_economy* economy READ get_economy CONSTANT)
	Q_PROPERTY(metternich::domain_government* government READ get_government CONSTANT)
	Q_PROPERTY(metternich::domain_military* military READ get_military CONSTANT)
	Q_PROPERTY(metternich::domain_technology* technology READ get_technology CONSTANT)
	Q_PROPERTY(metternich::domain_tier tier READ get_tier NOTIFY tier_changed)
	Q_PROPERTY(QString name READ get_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString title_name READ get_title_name_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString form_of_address READ get_form_of_address_qstring NOTIFY title_name_changed)
	Q_PROPERTY(QString flag READ get_flag_qstring NOTIFY title_name_changed)
	Q_PROPERTY(const metternich::culture* culture READ get_culture NOTIFY culture_changed)
	Q_PROPERTY(const metternich::religion* religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(const metternich::domain* realm READ get_realm NOTIFY realm_changed)
	Q_PROPERTY(QString type_name READ get_type_name_qstring NOTIFY type_name_changed)
	Q_PROPERTY(const metternich::government_type *government_type READ get_government_type NOTIFY government_type_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList sites READ get_sites_qvariant_list NOTIFY sites_changed)
	Q_PROPERTY(const metternich::site* capital READ get_capital NOTIFY capital_changed)
	Q_PROPERTY(int holding_count READ get_holding_count NOTIFY holding_count_changed)
	Q_PROPERTY(bool coastal READ is_coastal NOTIFY provinces_changed)
	Q_PROPERTY(bool anarchy READ is_under_anarchy NOTIFY provinces_changed)
	Q_PROPERTY(bool playable READ is_playable NOTIFY provinces_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QPoint territory_rect_center READ get_territory_rect_center NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList contiguous_territory_rects READ get_contiguous_territory_rects_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QRect main_contiguous_territory_rect READ get_main_contiguous_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QRect text_rect READ get_text_rect NOTIFY provinces_changed)
	Q_PROPERTY(QRect realm_territory_rect READ get_realm_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QRect realm_text_rect READ get_realm_text_rect NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList tile_terrain_counts READ get_tile_terrain_counts_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList attribute_values READ get_attribute_values_qvariant_list NOTIFY attribute_values_changed)
	Q_PROPERTY(QVariantList site_attribute_values READ get_site_attribute_values_qvariant_list NOTIFY site_attribute_values_changed)
	Q_PROPERTY(int consumption READ get_consumption_int NOTIFY consumption_changed)
	Q_PROPERTY(int unrest READ get_effective_unrest NOTIFY unrest_changed)
	Q_PROPERTY(int score READ get_score NOTIFY score_changed)
	Q_PROPERTY(int score_rank READ get_score_rank NOTIFY score_rank_changed)
	Q_PROPERTY(int domain_size READ get_domain_size NOTIFY domain_size_changed)
	Q_PROPERTY(int domain_power READ get_domain_power NOTIFY domain_power_changed)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(metternich::population* population READ get_population CONSTANT)
	Q_PROPERTY(metternich::population* country_population READ get_country_population CONSTANT)
	Q_PROPERTY(int max_current_constructions READ get_max_current_constructions NOTIFY max_current_constructions_changed)
	Q_PROPERTY(QVariantList item_slots READ get_item_slots_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList ideas READ get_ideas_qvariant_list NOTIFY ideas_changed)
	Q_PROPERTY(QVariantList appointed_ideas READ get_appointed_ideas_qvariant_list NOTIFY appointed_ideas_changed)
	Q_PROPERTY(QVariantList available_deity_slots READ get_available_deity_slots_qvariant_list NOTIFY available_idea_slots_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(QVariantList characters READ get_characters_qvariant_list NOTIFY characters_changed)
	Q_PROPERTY(QVariantList historical_rulers READ get_historical_rulers_qvariant_list NOTIFY characters_changed)
	Q_PROPERTY(QVariantList transporters READ get_transporters_qvariant_list NOTIFY transporters_changed)
	Q_PROPERTY(QVariantList active_journal_entries READ get_active_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(QVariantList inactive_journal_entries READ get_inactive_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(QVariantList finished_journal_entries READ get_finished_journal_entries_qvariant_list NOTIFY journal_entries_changed)
	Q_PROPERTY(qint64 maintenance_cost READ get_maintenance_cost NOTIFY maintenance_cost_changed)

public:
	static constexpr int first_deity_cost = 10;
	static constexpr int base_deity_cost = 200;
	static constexpr int deity_cost_increment = 100;
	static constexpr int base_max_current_constructions = 1;

	explicit domain_game_data(metternich::domain *domain);
	~domain_game_data();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	[[nodiscard]] QCoro::Task<void> apply_history(const QDate &start_date);
	void apply_ruler_history(const QDate &start_date);

	[[nodiscard]]
	QCoro::Task<void> do_turn();

	void collect_regency();
	[[nodiscard]] QCoro::Task<void> pay_maintenance();
	void check_item_slots();
	void do_civilian_unit_recruitment();
	void do_transporter_recruitment();
	[[nodiscard]] QCoro::Task<void> do_construction();
	[[nodiscard]] QCoro::Task<void> do_population_growth();
	void do_population_literacy_change();
	[[nodiscard]] QCoro::Task<void> do_population_cultural_change();
	[[nodiscard]] QCoro::Task<void> do_population_promotion();
	[[nodiscard]] QCoro::Task<void> do_population_employment();
	[[nodiscard]] QCoro::Task<void> do_events();

	domain_diplomacy *get_diplomacy() const
	{
		return this->diplomacy.get();
	}

	domain_economy *get_economy() const
	{
		return this->economy.get();
	}

	domain_government *get_government() const
	{
		return this->government.get();
	}

	domain_military *get_military() const
	{
		return this->military.get();
	}

	domain_technology *get_technology() const
	{
		return this->technology.get();
	}

	bool is_ai() const;
	domain_ai *get_ai() const;

	domain_tier get_tier() const
	{
		return this->tier;
	}

	[[nodiscard]] QCoro::Task<void> set_tier(const domain_tier tier);
	[[nodiscard]] QCoro::Task<void> check_tier();

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

	const std::string &get_form_of_address() const;

	QString get_form_of_address_qstring() const
	{
		return QString::fromStdString(this->get_form_of_address());
	}

	bool has_definite_article() const;

	const std::string &get_flag() const;

	QString get_flag_qstring() const
	{
		return QString::fromStdString(this->get_flag());
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	[[nodiscard]] QCoro::Task<void> set_culture(const metternich::culture *culture);
	[[nodiscard]] QCoro::Task<void> check_culture();
	bool is_culture_allowed(const metternich::culture *culture) const;

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	[[nodiscard]] QCoro::Task<void> set_religion(const metternich::religion *religion);
	[[nodiscard]] QCoro::Task<void> check_religion();

	const metternich::domain *get_realm() const;

	std::string get_type_name() const;

	QString get_type_name_qstring() const
	{
		return QString::fromStdString(this->get_type_name());
	}

	const metternich::government_type *get_government_type() const
	{
		return this->government_type;
	}

	[[nodiscard]] QCoro::Task<void> set_government_type(const metternich::government_type *government_type);
	bool can_have_government_type(const metternich::government_type *government_type) const;
	[[nodiscard]] QCoro::Task<void> check_government_type();

	bool is_tribal() const;
	bool is_clade() const;

	const dynasty *get_dynasty() const;

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;
	[[nodiscard]] QCoro::Task<void> add_province(const province *province);
	[[nodiscard]] QCoro::Task<void> remove_province(const province *province);
	[[nodiscard]] QCoro::Task<void> on_province_gained(const province *province, const int multiplier);

	int get_province_count() const
	{
		return static_cast<int>(this->get_provinces().size());
	}

	std::vector<const province *> get_accessible_provinces() const;

	const std::vector<const site *> &get_sites() const
	{
		return this->sites;
	}

	QVariantList get_sites_qvariant_list() const;
	[[nodiscard]] QCoro::Task<void> add_site(const site *site);
	[[nodiscard]] QCoro::Task<void> remove_site(const site *site);
	[[nodiscard]] QCoro::Task<void> on_site_gained(const site *site, const int multiplier);

	const site *get_capital() const
	{
		return this->capital;
	}

	[[nodiscard]] QCoro::Task<void> set_capital(const site *capital);
	[[nodiscard]] QCoro::Task<void> choose_capital();

	const province *get_capital_province() const;

	int get_holding_count() const
	{
		return this->holding_count;
	}

	[[nodiscard]] QCoro::Task<void> change_holding_count(const int change);
	int get_holding_count_with_vassals() const;

	const std::vector<const province *> &get_border_provinces() const
	{
		return this->border_provinces;
	}

	bool is_alive() const
	{
		return !this->get_provinces().empty() || !this->get_sites().empty();
	}

	bool is_under_anarchy() const
	{
		return this->get_capital() == nullptr;
	}

	bool is_playable() const;
	Q_INVOKABLE QString get_unplayable_reason() const;

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

	const QPoint &get_center_tile_pos() const
	{
		return this->center_tile_pos;
	}

	void calculate_center_tile_pos();

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
	QRect calculate_text_rect(const QRect &main_contiguous_territory_rect, const std::function<bool(const QPoint &)> &can_expand_func) const;

	const QRect &get_realm_territory_rect() const
	{
		return this->realm_territory_rect;
	}

	void calculate_realm_territory_rect();

	const std::vector<QRect> &get_realm_contiguous_territory_rects() const
	{
		return this->realm_contiguous_territory_rects;
	}

	const QRect &get_main_realm_contiguous_territory_rect() const
	{
		return this->main_realm_contiguous_territory_rect;
	}

	const QRect &get_realm_text_rect() const
	{
		return this->realm_text_rect;
	}

	void calculate_realm_text_rect();

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

	std::vector<const metternich::domain *> get_neighbor_countries() const;

	const data_entry_map<domain_attribute, decimillesimal_int> &get_attribute_values() const
	{
		return this->attribute_values;
	}

	QVariantList get_attribute_values_qvariant_list() const;

	const decimillesimal_int &get_attribute_value(const domain_attribute *attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end()) {
			return find_iterator->second;
		}

		static constexpr decimillesimal_int zero;
		return zero;
	}

	[[nodiscard]] QCoro::Task<void> change_attribute_value(const domain_attribute *attribute, const decimillesimal_int &change);
	bool do_attribute_check(const domain_attribute *attribute, const int roll_modifier, int *roll_result_output = nullptr) const;
	int get_attribute_check_chance(const domain_attribute *attribute, const int roll_modifier) const;
	int get_attribute_check_control_modifier() const;

	const data_entry_map<site_attribute, int> &get_site_attribute_values() const
	{
		return this->site_attribute_values;
	}

	QVariantList get_site_attribute_values_qvariant_list() const;

	int get_site_attribute_value(const site_attribute *attribute) const
	{
		const auto find_iterator = this->site_attribute_values.find(attribute);
		if (find_iterator != this->site_attribute_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	[[nodiscard]] QCoro::Task<void> change_site_attribute_value(const site_attribute *attribute, const int change);

	const centesimal_int &get_consumption() const
	{
		return this->consumption;
	}

	int get_consumption_int() const
	{
		return this->get_consumption().to_int();
	}

	void set_consumption(const centesimal_int &consumption);

	void change_consumption(const centesimal_int &change)
	{
		this->set_consumption(this->get_consumption() + change);
	}

	int get_unrest() const
	{
		return this->unrest;
	}

	void set_unrest(const int unrest);

	void change_unrest(const int change)
	{
		this->set_unrest(this->get_unrest() + change);
	}

	int get_effective_unrest() const
	{
		return std::max(this->get_unrest(), 0);
	}

	void change_province_loyalty(const int change);
	[[nodiscard]] QCoro::Task<void> check_rebellions();

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

	const domain_rank *get_rank() const
	{
		return this->rank;
	}

	void set_rank(const domain_rank *rank)
	{
		if (rank == this->get_rank()) {
			return;
		}

		this->rank = rank;
		emit rank_changed();
	}

	int get_domain_size() const
	{
		return this->domain_size;
	}

	void change_domain_size(const int change);

	int get_domain_power() const
	{
		return this->domain_power;
	}

	void change_domain_power(const int change);

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

	metternich::population *get_country_population() const
	{
		return this->country_population.get();
	}

	void on_population_unit_gained(const population_unit *population_unit, const int multiplier);

	phenotype_map<int64_t> get_phenotype_weights() const;

	Q_INVOKABLE const icon *get_population_type_small_icon(const metternich::population_type *type) const;

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

	[[nodiscard]] QCoro::Task<void> change_settlement_building_count(const building_type *building, const int change);

	[[nodiscard]] QCoro::Task<void> on_wonder_gained(const wonder *wonder, const int multiplier);

	[[nodiscard]] QCoro::Task<bool> choose_construction();

	int get_max_current_constructions() const
	{
		return this->max_current_constructions;
	}

	void set_max_current_constructions(const int max);

	void change_max_current_constructions(const int change)
	{
		this->set_max_current_constructions(this->get_max_current_constructions() + change);
	}

	std::vector<building_item_slot *> get_item_slots() const;
	QVariantList get_item_slots_qvariant_list() const;

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

	const scripted_domain_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_domain_modifier *modifier) const;
	[[nodiscard]] QCoro::Task<void> add_scripted_modifier(const scripted_domain_modifier *modifier, const int duration);
	[[nodiscard]] QCoro::Task<void> remove_scripted_modifier(const scripted_domain_modifier *modifier);
	[[nodiscard]] QCoro::Task<void> decrement_scripted_modifiers();

	[[nodiscard]] QCoro::Task<void> apply_modifier(const modifier<const metternich::domain> *modifier, const int multiplier = 1);

	[[nodiscard]] QCoro::Task<void> remove_modifier(const modifier<const metternich::domain> *modifier)
	{
		co_await this->apply_modifier(modifier, -1);
	}

	const std::vector<const character *> &get_characters() const;
	QVariantList get_characters_qvariant_list() const;
	void add_character(const character *character);
	void remove_character(const character *character);
	[[nodiscard]] QCoro::Task<void> check_characters();
	[[nodiscard]] QCoro::Task<void> on_character_recruited(const character *character);

	[[nodiscard]] QCoro::Task<const character *> generate_character(const std::vector<const character_class *> &allowed_character_classes, const std::vector<const monster_type *> &allowed_monster_types, const int level, const gender gender);
	[[nodiscard]] QCoro::Task<void> generate_ruler();

	const std::map<QDate, const character *> &get_historical_rulers() const
	{
		return this->historical_rulers;
	}

	QVariantList get_historical_rulers_qvariant_list() const;

	void set_historical_rulers(const std::map<QDate, const character *> &historical_rulers)
	{
		this->historical_rulers = historical_rulers;
	}

	void add_historical_ruler(const character *character);
	QDate get_historical_ruler_start_date(const character *character) const;
	QDate get_historical_ruler_end_date(const character *character) const;

	const std::map<QDate, const character *> &get_historical_monarchs() const
	{
		return this->historical_monarchs;
	}

	void set_historical_monarchs(const std::map<QDate, const character *> &historical_monarchs)
	{
		this->historical_monarchs = historical_monarchs;
	}

	const std::vector<qunique_ptr<civilian_unit>> &get_civilian_units() const
	{
		return this->civilian_units;
	}

	bool create_civilian_unit(const civilian_unit_type *civilian_unit_type, const province *deployment_province, const phenotype *phenotype);
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
	commodity_map<int64_t> get_transporter_type_commodity_costs(const transporter_type *transporter_type, const int quantity) const;
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

	[[nodiscard]] QCoro::Task<void> set_population_type_modifier_multiplier(const population_type *type, const centesimal_int &value);

	[[nodiscard]] QCoro::Task<void> change_population_type_modifier_multiplier(const population_type *type, const centesimal_int &change)
	{
		co_await this->set_population_type_modifier_multiplier(type, this->get_population_type_modifier_multiplier(type) + change);
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

	Q_INVOKABLE bool is_tile_explored(const QPoint &tile_pos) const;
	bool is_province_discovered(const province *province) const;

	const province_set &get_explored_provinces() const
	{
		return this->explored_provinces;
	}

	Q_INVOKABLE bool is_province_explored(const metternich::province *province) const;

	bool is_region_discovered(const region *region) const;

	[[nodiscard]] QCoro::Task<void> explore_province(const province *province);

	const point_set &get_prospected_tiles() const
	{
		return this->prospected_tiles;
	}

	bool is_tile_prospected(const QPoint &tile_pos) const
	{
		return this->prospected_tiles.contains(tile_pos);
	}

	[[nodiscard]] QCoro::Task<void> prospect_tile(const QPoint &tile_pos);
	void reset_tile_prospection(const QPoint &tile_pos);

	const std::vector<const journal_entry *> &get_active_journal_entries() const
	{
		return this->active_journal_entries;
	}

	QVariantList get_active_journal_entries_qvariant_list() const;
	[[nodiscard]] QCoro::Task<void> add_active_journal_entry(const journal_entry *journal_entry);
	[[nodiscard]] QCoro::Task<void> remove_active_journal_entry(const journal_entry *journal_entry);

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
	[[nodiscard]] QCoro::Task<void> check_journal_entries(const bool ignore_effects = false, const bool ignore_random_chance = false);
	bool check_potential_journal_entries();
	[[nodiscard]] QCoro::Task<bool> check_inactive_journal_entries();

	[[nodiscard]]
	QCoro::Task<bool> check_active_journal_entries(const read_only_context &ctx, const bool ignore_effects, const bool ignore_random_chance);

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

	[[nodiscard]] QCoro::Task<void> set_free_building_class_count(const building_class *building_class, const int value);

	[[nodiscard]] QCoro::Task<void> change_free_building_class_count(const building_class *building_class, const int value)
	{
		co_await this->set_free_building_class_count(building_class, this->get_free_building_class_count(building_class) + value);
	}

	int64_t get_domain_maintenance_cost() const;
	int64_t get_maintenance_cost() const;

	bool can_form_domain(const metternich::domain *other) const;
	bool can_form_domain_by_culture(const metternich::domain *other) const;
	bool can_form_domain_by_territory(const metternich::domain *other) const;
	bool can_release_domain(const metternich::domain *other, const bool count_only_rebellious_provinces) const;
	[[nodiscard]] QCoro::Task<void> release_domain(const metternich::domain *releasable_domain, const bool include_only_rebellious_provinces);
	bool has_domain_cores(const metternich::domain *other, const bool count_only_rebellious_provinces) const;

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

	QPromise<void> *get_construction_chosen_promise()
	{
		return this->construction_chosen_promise.get();
	}

	Q_INVOKABLE bool can_visit_site(const metternich::site *site) const;

signals:
	void tier_changed();
	void title_name_changed();
	void culture_changed();
	void religion_changed();
	void realm_changed();
	void type_name_changed();
	void government_type_changed();
	void provinces_changed();
	void sites_changed();
	void capital_changed();
	void holding_count_changed();
	void attribute_values_changed();
	void site_attribute_values_changed();
	void consumption_changed();
	void unrest_changed();
	void score_changed();
	void score_rank_changed();
	void rank_changed();
	void domain_size_changed();
	void domain_power_changed();
	void population_units_changed();
	void population_type_inputs_changed();
	void population_type_outputs_changed();
	void settlement_building_counts_changed();
	void max_current_constructions_changed();
	void building_built(const building_type *building, const site *site);
	void pathway_built(const pathway *pathway, const province *province);
	void item_slots_changed();
	void ideas_changed();
	void appointed_ideas_changed();
	void available_idea_slots_changed();
	void scripted_modifiers_changed();
	void characters_changed();
	void transporters_changed();
	void prospected_tiles_changed();
	void journal_entries_changed();
	void journal_entry_completed(const journal_entry *journal_entry);
	void maintenance_cost_changed();

private:
	metternich::domain *domain = nullptr;
	domain_tier tier {};
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::government_type *government_type = nullptr;
	std::vector<const province *> provinces;
	std::vector<const site *> sites;
	const site *capital = nullptr;
	int holding_count = 0; //only includes built holdings
	std::vector<const province *> border_provinces;
	int coastal_province_count = 0;
	QRect territory_rect;
	QPoint territory_rect_center = QPoint(-1, -1);
	QPoint center_tile_pos = QPoint(-1, -1);
	std::vector<QRect> contiguous_territory_rects;
	QRect main_contiguous_territory_rect;
	QRect text_rect;
	QRect realm_territory_rect;
	std::vector<QRect> realm_contiguous_territory_rects;
	QRect main_realm_contiguous_territory_rect;
	QRect realm_text_rect;
	terrain_type_map<int> tile_terrain_counts;
	data_entry_map<domain_attribute, decimillesimal_int> attribute_values;
	data_entry_map<site_attribute, int> site_attribute_values;
	centesimal_int consumption;
	int unrest = 0;
	int score = 0;
	const domain_rank *rank = nullptr;
	int score_rank = 0;
	int economic_score = 0;
	int military_score = 0;
	int domain_size = 0; //number of provinces and holdings owned by the domain
	int domain_power = 0;
	std::vector<population_unit *> population_units;
	qunique_ptr<metternich::population> population;
	qunique_ptr<metternich::population> country_population; //population in the domain's provinces
	building_type_map<int> settlement_building_counts;
	int max_current_constructions = domain_game_data::base_max_current_constructions;
	std::map<idea_type, data_entry_map<idea_slot, const idea *>> ideas;
	std::map<idea_type, data_entry_map<idea_slot, const idea *>> appointed_ideas;
	scripted_domain_modifier_map<int> scripted_modifiers;
	std::vector<const character *> characters;
	std::map<QDate, const character *> historical_rulers;
	std::map<QDate, const character *> historical_monarchs; //used for regnal numbers
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
	province_set explored_provinces;
	point_set prospected_tiles;
	std::vector<const journal_entry *> active_journal_entries;
	std::vector<const journal_entry *> inactive_journal_entries;
	std::vector<const journal_entry *> finished_journal_entries;
	building_class_map<int> free_building_class_counts;
	std::set<const flag *> flags;
	std::unique_ptr<QPromise<void>> construction_chosen_promise;
	qunique_ptr<domain_diplomacy> diplomacy;
	qunique_ptr<domain_economy> economy;
	qunique_ptr<domain_government> government;
	qunique_ptr<domain_military> military;
	qunique_ptr<domain_technology> technology;
};

}
