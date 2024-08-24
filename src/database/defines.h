#pragma once

#include "database/defines_base.h"
#include "economy/commodity_container.h"
#include "map/terrain_adjacency.h"
#include "util/fractional_int.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("economy/commodity.h")
Q_MOC_INCLUDE("infrastructure/pathway.h")
Q_MOC_INCLUDE("map/terrain_type.h")
Q_MOC_INCLUDE("population/population_class.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace archimedes {
	enum class log_level;
}

namespace metternich {

class building_class;
class commodity;
class icon;
class pathway;
class population_class;
class portrait;
class terrain_type;
enum class diplomacy_state;
enum class event_trigger;

class defines final : public defines_base, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(archimedes::log_level min_log_level MEMBER min_log_level READ get_min_log_level NOTIFY changed)
	Q_PROPERTY(QColor green_text_color MEMBER green_text_color READ get_green_text_color NOTIFY changed)
	Q_PROPERTY(QColor red_text_color MEMBER red_text_color READ get_red_text_color NOTIFY changed)
	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size NOTIFY changed)
	Q_PROPERTY(QSize scaled_tile_size READ get_scaled_tile_size NOTIFY scaled_tile_size_changed)
	Q_PROPERTY(int default_months_per_turn MEMBER default_months_per_turn NOTIFY changed)
	Q_PROPERTY(QDate default_start_date MEMBER default_start_date READ get_default_start_date)
	Q_PROPERTY(metternich::terrain_type* default_base_terrain MEMBER default_base_terrain)
	Q_PROPERTY(metternich::terrain_type* unexplored_terrain MEMBER unexplored_terrain)
	Q_PROPERTY(metternich::terrain_type* default_province_terrain MEMBER default_province_terrain)
	Q_PROPERTY(metternich::terrain_type* default_water_zone_terrain MEMBER default_water_zone_terrain)
	Q_PROPERTY(metternich::pathway* route_pathway MEMBER route_pathway NOTIFY changed)
	Q_PROPERTY(metternich::population_class* default_population_class MEMBER default_population_class)
	Q_PROPERTY(metternich::population_class* default_tribal_population_class MEMBER default_tribal_population_class)
	Q_PROPERTY(metternich::population_class* default_literate_population_class MEMBER default_literate_population_class)
	Q_PROPERTY(int population_per_unit MEMBER population_per_unit READ get_population_per_unit)
	Q_PROPERTY(int population_growth_threshold MEMBER population_growth_threshold READ get_population_growth_threshold NOTIFY changed)
	Q_PROPERTY(const metternich::commodity* research_commodity MEMBER research_commodity NOTIFY changed)
	Q_PROPERTY(const metternich::commodity* prestige_commodity MEMBER prestige_commodity NOTIFY changed)
	Q_PROPERTY(const metternich::commodity* piety_commodity MEMBER piety_commodity NOTIFY changed)
	Q_PROPERTY(const metternich::commodity* tradition_commodity MEMBER tradition_commodity NOTIFY changed)
	Q_PROPERTY(const metternich::commodity* advisor_commodity MEMBER advisor_commodity NOTIFY changed)
	Q_PROPERTY(const metternich::commodity* leader_commodity MEMBER leader_commodity NOTIFY changed)
	Q_PROPERTY(const metternich::icon* tariff_icon MEMBER tariff_icon NOTIFY changed)
	Q_PROPERTY(const metternich::icon* treasure_fleet_icon MEMBER treasure_fleet_icon NOTIFY changed)
	Q_PROPERTY(const metternich::icon* military_upkeep_icon MEMBER military_upkeep_icon NOTIFY changed)
	Q_PROPERTY(metternich::portrait* interior_minister_portrait MEMBER interior_minister_portrait NOTIFY changed)
	Q_PROPERTY(metternich::portrait* war_minister_portrait MEMBER war_minister_portrait NOTIFY changed)
	Q_PROPERTY(QColor minor_nation_color MEMBER minor_nation_color READ get_minor_nation_color NOTIFY changed)
	Q_PROPERTY(QColor country_border_color MEMBER country_border_color READ get_country_border_color NOTIFY changed)
	Q_PROPERTY(QColor selected_country_color MEMBER selected_country_color READ get_selected_country_color NOTIFY changed)
	Q_PROPERTY(QColor ocean_color MEMBER ocean_color READ get_ocean_color NOTIFY changed)
	Q_PROPERTY(QColor minimap_ocean_color MEMBER minimap_ocean_color READ get_minimap_ocean_color NOTIFY changed)
	Q_PROPERTY(std::filesystem::path river_image_filepath MEMBER river_image_filepath WRITE set_river_image_filepath)
	Q_PROPERTY(std::filesystem::path rivermouth_image_filepath MEMBER rivermouth_image_filepath WRITE set_rivermouth_image_filepath)
	Q_PROPERTY(std::filesystem::path province_border_image_filepath MEMBER province_border_image_filepath WRITE set_province_border_image_filepath)
	Q_PROPERTY(QString default_menu_background_filepath READ get_default_menu_background_filepath_qstring NOTIFY changed)
	Q_PROPERTY(int min_diplomatic_map_tile_scale MEMBER min_diplomatic_map_tile_scale READ get_min_diplomatic_map_tile_scale NOTIFY changed)

public:
	defines();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	log_level get_min_log_level() const
	{
		return this->min_log_level;
	}

	const QColor &get_green_text_color() const
	{
		return this->green_text_color;
	}

	const QColor &get_red_text_color() const
	{
		return this->red_text_color;
	}

	const QSize &get_tile_size() const
	{
		return this->tile_size;
	}

	int get_tile_width() const
	{
		return this->get_tile_size().width();
	}

	int get_tile_height() const
	{
		return this->get_tile_size().height();
	}

	QSize get_scaled_tile_size() const;
	int get_scaled_tile_width() const;
	int get_scaled_tile_height() const;

	int get_months_per_turn(const int current_year) const
	{
		auto find_iterator = this->months_per_turn_from_year.upper_bound(current_year);
		if (find_iterator != this->months_per_turn_from_year.end()) {
			if (find_iterator != this->months_per_turn_from_year.begin()) {
				--find_iterator; //get the one just before
				return find_iterator->second;
			}
		}

		return this->default_months_per_turn;
	}

	centesimal_int days_to_turns(const int days, const int current_year) const
	{
		return centesimal_int(days) / 30 / this->get_months_per_turn(current_year);
	}

	centesimal_int months_to_turns(const centesimal_int &months, const int current_year) const
	{
		return months / this->get_months_per_turn(current_year);
	}

	centesimal_int months_to_turns(const int months, const int current_year) const
	{
		return this->months_to_turns(centesimal_int(months), current_year);
	}

	centesimal_int years_to_turns(const int years, const int current_year) const
	{
		return this->months_to_turns(years * 12, current_year);
	}

	const QDate &get_default_start_date() const
	{
		return this->default_start_date;
	}

	const terrain_type *get_default_base_terrain() const
	{
		return this->default_base_terrain;
	}

	const terrain_type *get_unexplored_terrain() const
	{
		return this->unexplored_terrain;
	}

	const terrain_type *get_default_province_terrain() const
	{
		return this->default_province_terrain;
	}

	const terrain_type *get_default_water_zone_terrain() const
	{
		return this->default_water_zone_terrain;
	}

	const pathway *get_route_pathway() const
	{
		return this->route_pathway;
	}

	const population_class *get_default_population_class() const
	{
		return this->default_population_class;
	}

	const population_class *get_default_tribal_population_class() const
	{
		return this->default_tribal_population_class;
	}

	const population_class *get_default_literate_population_class() const
	{
		return this->default_literate_population_class;
	}

	int get_population_per_unit() const
	{
		return this->population_per_unit;
	}

	int get_population_growth_threshold() const
	{
		return this->population_growth_threshold;
	}

	const commodity_map<int> &get_settlement_commodity_bonuses() const
	{
		return this->settlement_commodity_bonuses;
	}

	const commodity_map<int> &get_river_settlement_commodity_bonuses() const
	{
		return this->river_settlement_commodity_bonuses;
	}

	const commodity *get_research_commodity() const
	{
		return this->research_commodity;
	}

	const commodity *get_prestige_commodity() const
	{
		return this->prestige_commodity;
	}

	const commodity *get_piety_commodity() const
	{
		return this->piety_commodity;
	}

	const commodity *get_tradition_commodity() const
	{
		return this->tradition_commodity;
	}

	const commodity *get_advisor_commodity() const
	{
		return this->advisor_commodity;
	}

	const commodity *get_leader_commodity() const
	{
		return this->leader_commodity;
	}

	const icon *get_tariff_icon() const
	{
		return this->tariff_icon;
	}

	const icon *get_treasure_fleet_icon() const
	{
		return this->treasure_fleet_icon;
	}

	const icon *get_military_upkeep_icon() const
	{
		return this->military_upkeep_icon;
	}

	const portrait *get_interior_minister_portrait() const
	{
		return this->interior_minister_portrait;
	}

	const portrait *get_war_minister_portrait() const
	{
		return this->war_minister_portrait;
	}

	const QColor &get_minor_nation_color() const
	{
		return this->minor_nation_color;
	}

	const QColor &get_country_border_color() const
	{
		return this->country_border_color;
	}

	const QColor &get_selected_country_color() const
	{
		return this->selected_country_color;
	}

	const QColor &get_ocean_color() const
	{
		return this->ocean_color;
	}

	const QColor &get_minimap_ocean_color() const
	{
		return this->minimap_ocean_color;
	}

	const QColor &get_diplomacy_state_color(const diplomacy_state state) const
	{
		const auto find_iterator = this->diplomacy_state_colors.find(state);
		if (find_iterator != this->diplomacy_state_colors.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("Failed to get color for diplomacy state: " + static_cast<int>(state));
	}

	const std::filesystem::path &get_river_image_filepath() const
	{
		return this->river_image_filepath;
	}

	void set_river_image_filepath(const std::filesystem::path &filepath);

	const std::filesystem::path &get_rivermouth_image_filepath() const
	{
		return this->rivermouth_image_filepath;
	}

	void set_rivermouth_image_filepath(const std::filesystem::path &filepath);

	const std::filesystem::path &get_province_border_image_filepath() const
	{
		return this->province_border_image_filepath;
	}

	void set_province_border_image_filepath(const std::filesystem::path &filepath);

	const std::map<event_trigger, int> get_event_trigger_none_random_weights() const
	{
		return this->event_trigger_none_random_weights;
	}

	QString get_default_menu_background_filepath_qstring() const;
	void set_default_menu_background_filepath(const std::filesystem::path &filepath);

	Q_INVOKABLE void set_default_menu_background_filepath(const std::string &filepath)
	{
		this->set_default_menu_background_filepath(std::filesystem::path(filepath));
	}

	int get_min_diplomatic_map_tile_scale() const
	{
		return this->min_diplomatic_map_tile_scale;
	}

	const std::vector<int> &get_river_adjacency_subtiles(const terrain_adjacency &adjacency) const;
	void set_river_adjacency_subtiles(const terrain_adjacency &adjacency, const std::vector<int> &subtiles);
	int get_rivermouth_adjacency_tile(const terrain_adjacency &adjacency) const;
	void set_rivermouth_adjacency_tile(const terrain_adjacency &adjacency, const int tile);
	int get_route_adjacency_tile(const terrain_adjacency &adjacency) const;
	void set_route_adjacency_tile(const terrain_adjacency &adjacency, const int tile);

signals:
	void changed();
	void scaled_tile_size_changed();

private:
	log_level min_log_level;
	QColor green_text_color;
	QColor red_text_color;
	QSize tile_size = QSize(64, 64);
	int default_months_per_turn = 3;
	std::map<int, int> months_per_turn_from_year;
	QDate default_start_date;
	terrain_type *default_base_terrain = nullptr;
	terrain_type *unexplored_terrain = nullptr;
	terrain_type *default_province_terrain = nullptr;
	terrain_type *default_water_zone_terrain = nullptr;
	pathway *route_pathway = nullptr;
	population_class *default_population_class = nullptr;
	population_class *default_tribal_population_class = nullptr;
	population_class *default_literate_population_class = nullptr;
	int population_per_unit = 10000;
	int population_growth_threshold = 100;
	commodity_map<int> settlement_commodity_bonuses;
	commodity_map<int> river_settlement_commodity_bonuses;
	const commodity *research_commodity = nullptr;
	const commodity *prestige_commodity = nullptr;
	const commodity *piety_commodity = nullptr;
	const commodity *tradition_commodity = nullptr;
	const commodity *advisor_commodity = nullptr;
	const commodity *leader_commodity = nullptr;
	const icon *tariff_icon = nullptr;
	const icon *treasure_fleet_icon = nullptr;
	const icon *military_upkeep_icon = nullptr;
	portrait *interior_minister_portrait = nullptr;
	portrait *war_minister_portrait = nullptr;
	QColor minor_nation_color;
	QColor country_border_color;
	QColor selected_country_color;
	QColor ocean_color;
	QColor minimap_ocean_color;
	std::map<diplomacy_state, QColor> diplomacy_state_colors;
	std::filesystem::path river_image_filepath;
	std::filesystem::path rivermouth_image_filepath;
	std::filesystem::path route_image_filepath;
	std::filesystem::path province_border_image_filepath;
	std::map<event_trigger, int> event_trigger_none_random_weights; //the weight for no event happening for a given event trigger's random event selection
	std::filesystem::path default_menu_background_filepath;
	int min_diplomatic_map_tile_scale = 2;
	std::map<terrain_adjacency, std::vector<int>> river_adjacency_subtiles;
	std::map<terrain_adjacency, int> rivermouth_adjacency_tiles;
	std::map<terrain_adjacency, int> route_adjacency_tiles;
};

}
