#pragma once

#include "character/office_container.h"
#include "country/country_container.h"
#include "country/culture_container.h"
#include "country/religion_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "map/terrain_type_container.h"
#include "population/phenotype_container.h"
#include "population/population_type_container.h"
#include "technology/technology_container.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

namespace metternich {

class character;
class civilian_unit;
class consulate;
class country;
class culture;
class event;
class population_unit;
class province;
class religion;
enum class diplomacy_state;
enum class diplomatic_map_mode;
enum class event_trigger;
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
	Q_PROPERTY(QVariantList population_type_counts READ get_population_type_counts_qvariant_list NOTIFY population_type_counts_changed)
	Q_PROPERTY(QVariantList population_culture_counts READ get_population_culture_counts_qvariant_list NOTIFY population_culture_counts_changed)
	Q_PROPERTY(QVariantList population_religion_counts READ get_population_religion_counts_qvariant_list NOTIFY population_religion_counts_changed)
	Q_PROPERTY(QVariantList population_phenotype_counts READ get_population_phenotype_counts_qvariant_list NOTIFY population_phenotype_counts_changed)
	Q_PROPERTY(int population READ get_population NOTIFY population_changed)
	Q_PROPERTY(int population_growth READ get_population_growth NOTIFY population_growth_changed)
	Q_PROPERTY(int wealth READ get_wealth NOTIFY wealth_changed)
	Q_PROPERTY(int prestige READ get_prestige_int NOTIFY prestige_changed)
	Q_PROPERTY(int piety READ get_piety_int NOTIFY piety_changed)
	Q_PROPERTY(QVariantList stored_commodities READ get_stored_commodities_qvariant_list NOTIFY stored_commodities_changed)
	Q_PROPERTY(QColor diplomatic_map_color READ get_diplomatic_map_color NOTIFY overlord_changed)
	Q_PROPERTY(QVariantList characters READ get_characters_qvariant_list NOTIFY characters_changed)
	Q_PROPERTY(metternich::character* ruler READ get_ruler_unconst NOTIFY ruler_changed)
	Q_PROPERTY(QVariantList offices READ get_offices_qvariant_list NOTIFY offices_changed)

public:
	explicit country_game_data(metternich::country *country);
	~country_game_data();

	void do_turn();
	void do_population_growth();
	void do_migration();
	void do_events();
	void do_ai_turn();

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
	bool is_overlord_of(const metternich::country *country) const;
	bool is_any_overlord_of(const metternich::country *country) const;

	bool is_true_great_power() const;
	bool is_secondary_power() const;

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

	const province *get_random_population_weighted_province() const;

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

	std::vector<const metternich::country *> get_vassals() const;
	QVariantList get_vassals_qvariant_list() const;
	QVariantList get_colonies_qvariant_list() const;

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

	void add_population_unit(population_unit *population_unit)
	{
		this->population_units.push_back(population_unit);
	}

	void remove_population_unit(population_unit *population_unit)
	{
		std::erase(this->population_units, population_unit);
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

	void decrease_population();

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

	const centesimal_int &get_prestige() const
	{
		return this->prestige;
	}

	int get_prestige_int() const
	{
		return this->prestige.to_int();
	}

	void set_prestige(const centesimal_int &prestige)
	{
		if (prestige == this->get_prestige()) {
			return;
		}

		this->prestige = prestige;

		emit prestige_changed();
	}

	void change_prestige(const centesimal_int &change)
	{
		this->set_prestige(this->get_prestige() + change);
	}

	const centesimal_int &get_piety() const
	{
		return this->piety;
	}

	int get_piety_int() const
	{
		return this->get_piety().to_int();
	}

	void set_piety(const centesimal_int &piety)
	{
		if (piety == this->get_piety()) {
			return;
		}

		this->piety = piety;

		emit piety_changed();
	}

	void change_piety(const centesimal_int &change)
	{
		this->set_piety(this->get_piety() + change);
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

	void set_stored_commodity(const commodity *commodity, const int value);

	void change_stored_commodity(const commodity *commodity, const int value)
	{
		this->set_stored_commodity(commodity, this->get_stored_commodity(commodity) + value);
	}

	bool can_declare_war_on(const metternich::country *other_country) const;

	bool has_technology(const technology *technology) const
	{
		return this->technologies.contains(technology);
	}

	void add_technology(const technology *technology)
	{
		this->technologies.insert(technology);
	}

	const std::vector<const character *> &get_characters() const
	{
		return this->characters;
	}

	QVariantList get_characters_qvariant_list() const;
	void check_characters(const QDateTime &date);
	void add_character(const character *character);
	void remove_character(const character *character);
	void clear_characters();
	void sort_characters();

	const character *get_ruler() const
	{
		return this->ruler;
	}

private:
	//for the Qt property (pointers there can't be const)
	character *get_ruler_unconst() const
	{
		return const_cast<character *>(this->get_ruler());
	}

public:
	void set_ruler(const character *ruler);
	void apply_ruler_effects(const int multiplier);

	const office_map<const character *> &get_office_characters() const
	{
		return this->office_characters;
	}

	const character *get_office_character(const office *office) const
	{
		const auto find_iterator = this->office_characters.find(office);

		if (find_iterator != this->office_characters.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	Q_INVOKABLE QObject *get_office_character(metternich::office *office) const;

	void set_office_character(const office *office, const character *character);

	std::vector<const office *> get_offices() const;
	QVariantList get_offices_qvariant_list() const;
	void fill_empty_offices();

	void add_civilian_unit(qunique_ptr<metternich::civilian_unit> &&civilian_unit);
	void remove_civilian_unit(metternich::civilian_unit *civilian_unit);

	const centesimal_int &get_quarterly_prestige() const
	{
		return this->quarterly_prestige;
	}

	void change_quarterly_prestige(const centesimal_int &change)
	{
		this->quarterly_prestige += change;
	}

	const centesimal_int &get_quarterly_piety() const
	{
		return this->quarterly_piety;
	}

	void change_quarterly_piety(const centesimal_int &change)
	{
		this->quarterly_piety += change;
	}

	int get_land_morale_resistance() const
	{
		return this->land_morale_resistance;
	}

	void change_land_morale_resistance(const int change)
	{
		this->land_morale_resistance += change;
	}

	int get_naval_morale_resistance() const
	{
		return this->naval_morale_resistance;
	}

	void change_naval_morale_resistance(const int change)
	{
		this->naval_morale_resistance += change;
	}

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
	void population_type_counts_changed();
	void population_culture_counts_changed();
	void population_religion_counts_changed();
	void population_phenotype_counts_changed();
	void population_changed();
	void population_growth_changed();
	void wealth_changed();
	void prestige_changed();
	void piety_changed();
	void stored_commodities_changed();
	void characters_changed();
	void ruler_changed();
	void office_characters_changed();
	void offices_changed();

private:
	metternich::country *country = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::country *overlord = nullptr;
	std::vector<const province *> provinces;
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
	QImage diplomatic_map_image;
	QImage selected_diplomatic_map_image;
	std::map<diplomatic_map_mode, QImage> diplomatic_map_mode_images;
	std::map<diplomacy_state, QImage> diplomacy_state_diplomatic_map_images;
	QRect diplomatic_map_image_rect;
	int rank = 0;
	int score = 0;
	std::vector<population_unit *> population_units;
	population_type_map<int> population_type_counts;
	culture_map<int> population_culture_counts;
	religion_map<int> population_religion_counts;
	phenotype_map<int> population_phenotype_counts;
	int population = 0;
	int population_growth = 0; //population growth counter
	int wealth = 0;
	centesimal_int prestige;
	centesimal_int piety;
	commodity_map<int> stored_commodities;
	technology_set technologies;
	std::vector<const character *> characters;
	const character *ruler = nullptr;
	office_map<const character *> office_characters;
	std::vector<qunique_ptr<civilian_unit>> civilian_units;
	centesimal_int quarterly_prestige;
	centesimal_int quarterly_piety;
	int land_morale_resistance = 0;
	int naval_morale_resistance = 0;
};

}
