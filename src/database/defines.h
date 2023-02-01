#pragma once

#include "database/defines_base.h"
#include "map/terrain_adjacency.h"
#include "util/singleton.h"

namespace metternich {

class building_class;
class icon;
class office;
class population_class;
class terrain_type;
enum class attribute;
enum class diplomacy_state;
enum class event_trigger;

class defines final : public defines_base, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size NOTIFY changed)
	Q_PROPERTY(QSize scaled_tile_size READ get_scaled_tile_size NOTIFY scaled_tile_size_changed)
	Q_PROPERTY(int months_per_turn MEMBER months_per_turn READ get_months_per_turn NOTIFY changed)
	Q_PROPERTY(QDateTime default_start_date MEMBER default_start_date READ get_default_start_date)
	Q_PROPERTY(metternich::terrain_type* default_base_terrain MEMBER default_base_terrain)
	Q_PROPERTY(metternich::terrain_type* unexplored_terrain MEMBER unexplored_terrain)
	Q_PROPERTY(metternich::terrain_type* default_province_terrain MEMBER default_province_terrain)
	Q_PROPERTY(metternich::terrain_type* default_water_zone_terrain MEMBER default_water_zone_terrain)
	Q_PROPERTY(metternich::building_class* capitol_building_class MEMBER capitol_building_class)
	Q_PROPERTY(metternich::population_class* default_population_class MEMBER default_population_class)
	Q_PROPERTY(metternich::population_class* default_tribal_population_class MEMBER default_tribal_population_class)
	Q_PROPERTY(metternich::population_class* default_literate_population_class MEMBER default_literate_population_class)
	Q_PROPERTY(int population_per_unit MEMBER population_per_unit READ get_population_per_unit)
	Q_PROPERTY(int population_growth_threshold MEMBER population_growth_threshold READ get_population_growth_threshold NOTIFY changed)
	Q_PROPERTY(metternich::office* head_of_government_office MEMBER head_of_government_office NOTIFY changed)
	Q_PROPERTY(QColor minor_nation_color MEMBER minor_nation_color READ get_minor_nation_color NOTIFY changed)
	Q_PROPERTY(QColor country_border_color MEMBER country_border_color READ get_country_border_color)
	Q_PROPERTY(QColor selected_country_color MEMBER selected_country_color READ get_selected_country_color)
	Q_PROPERTY(QColor ocean_color MEMBER ocean_color READ get_ocean_color)
	Q_PROPERTY(QColor minimap_ocean_color MEMBER minimap_ocean_color READ get_minimap_ocean_color)
	Q_PROPERTY(std::filesystem::path default_settlement_image_filepath MEMBER default_settlement_image_filepath WRITE set_default_settlement_image_filepath)
	Q_PROPERTY(std::filesystem::path river_image_filepath MEMBER river_image_filepath WRITE set_river_image_filepath)
	Q_PROPERTY(std::filesystem::path rivermouth_image_filepath MEMBER rivermouth_image_filepath WRITE set_rivermouth_image_filepath)
	Q_PROPERTY(std::filesystem::path province_border_image_filepath MEMBER province_border_image_filepath WRITE set_province_border_image_filepath)
	Q_PROPERTY(QString default_menu_background_filepath READ get_default_menu_background_filepath_qstring NOTIFY changed)
	Q_PROPERTY(int min_diplomatic_map_tile_scale MEMBER min_diplomatic_map_tile_scale READ get_min_diplomatic_map_tile_scale NOTIFY changed)

public:
	defines();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

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

	int get_months_per_turn() const
	{
		return this->months_per_turn;
	}

	const QDateTime &get_default_start_date() const
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

	const building_class *get_capitol_building_class() const
	{
		return this->capitol_building_class;
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

	const office *get_head_of_government_office() const
	{
		return this->head_of_government_office;
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

	const std::filesystem::path &get_default_settlement_image_filepath() const
	{
		return this->default_settlement_image_filepath;
	}

	void set_default_settlement_image_filepath(const std::filesystem::path &filepath);

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

	const icon *get_attribute_icon(const attribute attribute) const
	{
		const auto find_iterator = this->attribute_icons.find(attribute);
		if (find_iterator != this->attribute_icons.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("Failed to get icon for attribute: " + static_cast<int>(attribute));
	}

	Q_INVOKABLE QString get_attribute_icon_identifier(const int attribute_index) const;

	int get_river_adjacency_tile(const terrain_adjacency &adjacency) const;
	void set_river_adjacency_tile(const terrain_adjacency &adjacency, const int tile);
	int get_rivermouth_adjacency_tile(const terrain_adjacency &adjacency) const;
	void set_rivermouth_adjacency_tile(const terrain_adjacency &adjacency, const int tile);

signals:
	void changed();
	void scaled_tile_size_changed();

private:
	QSize tile_size = QSize(64, 64);
	int months_per_turn = 3;
	QDateTime default_start_date;
	terrain_type *default_base_terrain = nullptr;
	terrain_type *unexplored_terrain = nullptr;
	terrain_type *default_province_terrain = nullptr;
	terrain_type *default_water_zone_terrain = nullptr;
	building_class *capitol_building_class = nullptr;
	population_class *default_population_class = nullptr;
	population_class *default_tribal_population_class = nullptr;
	population_class *default_literate_population_class = nullptr;
	int population_per_unit = 10000;
	int population_growth_threshold = 100;
	office *head_of_government_office = nullptr;
	QColor minor_nation_color;
	QColor country_border_color;
	QColor selected_country_color;
	QColor ocean_color;
	QColor minimap_ocean_color;
	std::map<diplomacy_state, QColor> diplomacy_state_colors;
	std::filesystem::path default_settlement_image_filepath;
	std::filesystem::path river_image_filepath;
	std::filesystem::path rivermouth_image_filepath;
	std::filesystem::path province_border_image_filepath;
	std::map<event_trigger, int> event_trigger_none_random_weights; //the weight for no event happening for a given event trigger's random event selection
	std::filesystem::path default_menu_background_filepath;
	int min_diplomatic_map_tile_scale = 2;
	std::map<attribute, const icon *> attribute_icons;
	std::map<terrain_adjacency, int> river_adjacency_tiles;
	std::map<terrain_adjacency, int> rivermouth_adjacency_tiles;
};

}
