#pragma once

#include "country/country_container.h"
#include "country/culture_container.h"
#include "country/ideology_container.h"
#include "country/religion_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "infrastructure/building_slot_type_container.h"
#include "map/terrain_type_container.h"
#include "population/phenotype_container.h"
#include "population/population_type_container.h"
#include "script/opinion_modifier_container.h"
#include "technology/technology_container.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

namespace metternich {

class building_slot;
class building_type;
class character;
class civilian_unit;
class consulate;
class country;
class culture;
class event;
class military_unit;
class military_unit_type;
class opinion_modifier;
class population_unit;
class province;
class religion;
class trait;
enum class diplomacy_state;
enum class diplomatic_map_mode;
enum class event_trigger;
enum class military_unit_category;
struct read_only_context;

class country_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::religion* religion READ get_religion_unconst NOTIFY religion_changed)
	Q_PROPERTY(metternich::country* overlord READ get_overlord_unconst NOTIFY overlord_changed)
	Q_PROPERTY(bool true_great_power READ is_true_great_power NOTIFY rank_changed)
	Q_PROPERTY(bool secondary_power READ is_secondary_power NOTIFY rank_changed)
	Q_PROPERTY(QString type_name READ get_type_name_qstring NOTIFY type_name_changed)
	Q_PROPERTY(QString vassalage_type_name READ get_vassalage_type_name_qstring NOTIFY vassalage_type_name_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
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
	Q_PROPERTY(QVariantList colonies READ get_colonies_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList consulates READ get_consulates_qvariant_list NOTIFY consulates_changed)
	Q_PROPERTY(QRect diplomatic_map_image_rect READ get_diplomatic_map_image_rect NOTIFY diplomatic_map_image_changed)
	Q_PROPERTY(int rank READ get_rank NOTIFY rank_changed)
	Q_PROPERTY(int score READ get_score NOTIFY score_changed)
	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_units_changed)
	Q_PROPERTY(QVariantList population_type_counts READ get_population_type_counts_qvariant_list NOTIFY population_type_counts_changed)
	Q_PROPERTY(QVariantList population_culture_counts READ get_population_culture_counts_qvariant_list NOTIFY population_culture_counts_changed)
	Q_PROPERTY(QVariantList population_religion_counts READ get_population_religion_counts_qvariant_list NOTIFY population_religion_counts_changed)
	Q_PROPERTY(QVariantList population_phenotype_counts READ get_population_phenotype_counts_qvariant_list NOTIFY population_phenotype_counts_changed)
	Q_PROPERTY(QVariantList population_ideology_counts READ get_population_ideology_counts_qvariant_list NOTIFY population_ideology_counts_changed)
	Q_PROPERTY(int population READ get_population NOTIFY population_changed)
	Q_PROPERTY(int population_growth READ get_population_growth NOTIFY population_growth_changed)
	Q_PROPERTY(QVariantList building_slots READ get_building_slots_qvariant_list CONSTANT)
	Q_PROPERTY(int wealth READ get_wealth NOTIFY wealth_changed)
	Q_PROPERTY(QVariantList stored_commodities READ get_stored_commodities_qvariant_list NOTIFY stored_commodities_changed)
	Q_PROPERTY(int storage_capacity READ get_storage_capacity NOTIFY storage_capacity_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)
	Q_PROPERTY(QVariantList technologies READ get_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList available_technologies READ get_available_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList future_technologies READ get_future_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QColor diplomatic_map_color READ get_diplomatic_map_color NOTIFY overlord_changed)
	Q_PROPERTY(QVariantList advisors READ get_advisors_qvariant_list NOTIFY advisors_changed)
	Q_PROPERTY(int advisor_cost READ get_advisor_cost NOTIFY advisors_changed)
	Q_PROPERTY(metternich::character* next_advisor READ get_next_advisor_unconst WRITE set_next_advisor NOTIFY next_advisor_changed)

public:
	static constexpr int base_advisor_cost = 80;

	explicit country_game_data(metternich::country *country);
	~country_game_data();

	void do_turn();
	void do_production();
	void do_population_growth();
	void do_cultural_change();
	void do_events();
	void do_ai_turn();

	bool is_ai() const;

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

	const metternich::country *get_overlord() const
	{
		return this->overlord;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_overlord_unconst() const
	{
		return const_cast<metternich::country *>(this->get_overlord());
	}

public:
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

	bool is_true_great_power() const;
	bool is_secondary_power() const;
	bool is_colony() const;

	std::string get_type_name() const;

	QString get_type_name_qstring() const
	{
		return QString::fromStdString(this->get_type_name());
	}

	std::string get_vassalage_type_name() const;

	QString get_vassalage_type_name_qstring() const
	{
		return QString::fromStdString(this->get_vassalage_type_name());
	}

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;
	void add_province(const province *province);
	void remove_province(const province *province);

	int get_province_count() const
	{
		return static_cast<int>(this->get_provinces().size());
	}

	const std::vector<const province *> &get_border_provinces() const
	{
		return this->border_provinces;
	}

	bool is_alive() const
	{
		return !this->get_provinces().empty();
	}

	bool is_under_anarchy() const;

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

	diplomacy_state get_diplomacy_state(const metternich::country *other_country) const;
	void set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state);

	const std::map<diplomacy_state, int> &get_diplomacy_state_counts() const
	{
		return this->diplomacy_state_counts;
	}

	void change_diplomacy_state_count(const diplomacy_state state, const int change);
	Q_INVOKABLE QString get_diplomacy_state_diplomatic_map_suffix(metternich::country *other_country) const;

	bool at_war() const;

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

	void set_base_opinion(const metternich::country *other, const int opinion)
	{
		if (opinion == this->get_base_opinion(other)) {
			return;
		}

		if (opinion == 0) {
			this->base_opinions.erase(other);
		} else {
			this->base_opinions[other] = opinion;
		}
	}

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

	std::vector<const metternich::country *> get_vassals() const;
	QVariantList get_vassals_qvariant_list() const;
	QVariantList get_colonies_qvariant_list() const;

	std::vector<const metternich::country *> get_neighbor_countries() const;

	const QColor &get_diplomatic_map_color() const;

	const QImage &get_diplomatic_map_image() const
	{
		return this->diplomatic_map_image;
	}

	[[nodiscard]]
	boost::asio::awaitable<void> create_diplomatic_map_image();

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
	boost::asio::awaitable<void> create_diplomatic_map_mode_image(const diplomatic_map_mode mode, const std::optional<diplomacy_state> &diplomacy_state);

	const QImage &get_diplomacy_state_diplomatic_map_image(const diplomacy_state state) const
	{
		const auto find_iterator = this->diplomacy_state_diplomatic_map_images.find(state);
		if (find_iterator != this->diplomacy_state_diplomatic_map_images.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("No diplomacy state diplomatic map image found for state " + std::to_string(static_cast<int>(state)) + ".");
	}

	int get_rank() const
	{
		return this->rank;
	}

	void set_rank(const int rank)
	{
		if (rank == this->get_rank()) {
			return;
		}

		this->rank = rank;
		emit rank_changed();
	}

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change);

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);
	qunique_ptr<population_unit> pop_population_unit(population_unit *population_unit);
	void create_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype);

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
	void change_population_culture_count(const culture *culture, const int change);

	const religion_map<int> &get_population_religion_counts() const
	{
		return this->population_religion_counts;
	}

	QVariantList get_population_religion_counts_qvariant_list() const;
	void change_population_religion_count(const religion *religion, const int change);

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

	Q_INVOKABLE QObject *get_population_type_small_icon(metternich::population_type *type) const;

	int get_food_consumption() const
	{
		return this->get_population_unit_count() + static_cast<int>(this->civilian_units.size()) + static_cast<int>(this->military_units.size());
	}

	QVariantList get_building_slots_qvariant_list() const;
	void initialize_building_slots();
	void initialize_free_buildings();

	const building_slot *get_building_slot(const building_slot_type *slot_type) const
	{
		const auto find_iterator = this->building_slot_map.find(slot_type);

		if (find_iterator != this->building_slot_map.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	const building_type *get_slot_building(const building_slot_type *slot_type) const;
	void set_slot_building(const building_slot_type *slot_type, const building_type *building);
	void clear_buildings();

	void on_building_gained(const building_type *building, const int multiplier);

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

	const commodity_map<int> &get_stored_commodities() const
	{
		return this->stored_commodities;
	}

	QVariantList get_stored_commodities_qvariant_list() const;

	int get_stored_commodity(const commodity *commodity) const
	{
		const auto find_iterator = this->stored_commodities.find(commodity);

		if (find_iterator != this->stored_commodities.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_stored_commodity(const QString &commodity_identifier) const;
	void set_stored_commodity(const commodity *commodity, const int value);

	void change_stored_commodity(const commodity *commodity, const int value)
	{
		this->set_stored_commodity(commodity, this->get_stored_commodity(commodity) + value);
	}

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

	Q_INVOKABLE int get_commodity_output(const QString &commodity_identifier) const;
	void change_commodity_output(const commodity *commodity, const int change);

	int get_net_commodity_output(const commodity *commodity) const
	{
		return this->get_commodity_output(commodity) - this->get_commodity_input(commodity);
	}

	void assign_production();
	void decrease_commodity_consumption(const commodity *commodity, const bool restore_inputs = true);

	bool can_declare_war_on(const metternich::country *other_country) const;

	QVariantList get_technologies_qvariant_list() const;

	bool has_technology(const technology *technology) const
	{
		return this->technologies.contains(technology);
	}

	void add_technology(const technology *technology);
	void add_technology_with_prerequisites(const technology *technology);

	QVariantList get_available_technologies_qvariant_list() const;
	QVariantList get_future_technologies_qvariant_list() const;

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
		if (this->get_advisors().empty()) {
			return country_game_data::base_advisor_cost / 2;
		}

		return country_game_data::base_advisor_cost * static_cast<int>(this->get_advisors().size() + 1);
	}

	const character *get_next_advisor() const
	{
		return this->next_advisor;
	}

private:
	//for the Qt property (pointers there can't be const)
	character *get_next_advisor_unconst() const
	{
		return const_cast<character *>(this->get_next_advisor());
	}

public:
	void set_next_advisor(const character *advisor)
	{
		if (advisor == this->get_next_advisor()) {
			return;
		}

		this->next_advisor = advisor;
		emit next_advisor_changed();
	}

	void choose_next_advisor();

	void add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit);
	void remove_civilian_unit(civilian_unit *civilian_unit);

	void add_military_unit(qunique_ptr<military_unit> &&civilian_unit);
	void remove_military_unit(military_unit *civilian_unit);

	const military_unit_type *get_best_military_unit_category_type(const military_unit_category category) const;

	int get_land_morale_resistance_modifier() const
	{
		return this->land_morale_resistance_modifier;
	}

	void change_land_morale_resistance_modifier(const int change)
	{
		this->land_morale_resistance_modifier += change;
	}

	int get_naval_morale_resistance_modifier() const
	{
		return this->naval_morale_resistance_modifier;
	}

	void change_naval_morale_resistance_modifier(const int change)
	{
		this->naval_morale_resistance_modifier += change;
	}

	int get_air_morale_resistance_modifier() const
	{
		return this->air_morale_resistance_modifier;
	}

	void change_air_morale_resistance_modifier(const int change)
	{
		this->air_morale_resistance_modifier += change;
	}

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

	void set_commodity_production_modifier(const commodity *commodity, const int value);

	void change_commodity_production_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_production_modifier(commodity, this->get_commodity_production_modifier(commodity) + value);
	}

	void decrement_scripted_modifiers();

signals:
	void religion_changed();
	void overlord_changed();
	void type_name_changed();
	void vassalage_type_name_changed();
	void diplomacy_states_changed();
	void consulates_changed();
	void provinces_changed();
	void diplomatic_map_image_changed();
	void rank_changed();
	void score_changed();
	void population_units_changed();
	void population_type_counts_changed();
	void population_culture_counts_changed();
	void population_religion_counts_changed();
	void population_phenotype_counts_changed();
	void population_ideology_counts_changed();
	void population_changed();
	void population_growth_changed();
	void wealth_changed();
	void stored_commodities_changed();
	void storage_capacity_changed();
	void technologies_changed();
	void advisors_changed();
	void next_advisor_changed();
	void commodity_inputs_changed();
	void commodity_outputs_changed();

private:
	metternich::country *country = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::country *overlord = nullptr;
	std::vector<const province *> provinces;
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
	country_map<diplomacy_state> diplomacy_states;
	std::map<diplomacy_state, int> diplomacy_state_counts;
	country_map<const consulate *> consulates;
	country_map<int> base_opinions;
	country_map<opinion_modifier_map<int>> opinion_modifiers;
	QImage diplomatic_map_image;
	QImage selected_diplomatic_map_image;
	std::map<diplomatic_map_mode, QImage> diplomatic_map_mode_images;
	std::map<diplomacy_state, QImage> diplomacy_state_diplomatic_map_images;
	QRect diplomatic_map_image_rect;
	int rank = 0;
	int score = 0;
	std::vector<qunique_ptr<population_unit>> population_units;
	population_type_map<int> population_type_counts;
	culture_map<int> population_culture_counts;
	religion_map<int> population_religion_counts;
	phenotype_map<int> population_phenotype_counts;
	ideology_map<int> population_ideology_counts;
	int population = 0;
	int population_growth = 0; //population growth counter
	std::vector<qunique_ptr<building_slot>> building_slots;
	building_slot_type_map<building_slot *> building_slot_map;
	int wealth = 0;
	commodity_map<int> stored_commodities;
	int storage_capacity = 0;
	commodity_map<int> commodity_inputs;
	commodity_map<int> commodity_outputs;
	technology_set technologies;
	std::vector<const character *> advisors;
	const character *next_advisor = nullptr;
	std::vector<qunique_ptr<civilian_unit>> civilian_units;
	std::vector<qunique_ptr<military_unit>> military_units;
	int land_morale_resistance_modifier = 0;
	int naval_morale_resistance_modifier = 0;
	int air_morale_resistance_modifier = 0;
	int production_modifier = 0;
	commodity_map<int> commodity_production_modifiers;
};

}
