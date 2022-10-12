#pragma once

#include "database/defines_base.h"
#include "util/singleton.h"

namespace metternich {

class terrain_type;

class defines final : public defines_base, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size NOTIFY changed)
	Q_PROPERTY(QSize scaled_tile_size READ get_scaled_tile_size NOTIFY scaled_tile_size_changed)
	Q_PROPERTY(metternich::terrain_type* default_base_terrain MEMBER default_base_terrain)
	Q_PROPERTY(metternich::terrain_type* unexplored_terrain MEMBER unexplored_terrain)
	Q_PROPERTY(metternich::terrain_type* default_province_terrain MEMBER default_province_terrain)
	Q_PROPERTY(metternich::terrain_type* default_water_zone_terrain MEMBER default_water_zone_terrain)
	Q_PROPERTY(QColor minor_nation_color MEMBER minor_nation_color READ get_minor_nation_color)
	Q_PROPERTY(QColor country_border_color MEMBER country_border_color READ get_country_border_color)
	Q_PROPERTY(std::filesystem::path default_settlement_image_filepath MEMBER default_settlement_image_filepath WRITE set_default_settlement_image_filepath)
	Q_PROPERTY(QString default_menu_background_filepath READ get_default_menu_background_filepath_qstring NOTIFY changed)
	Q_PROPERTY(int min_diplomatic_map_tile_scale MEMBER min_diplomatic_map_tile_scale READ get_min_diplomatic_map_tile_scale NOTIFY changed)

public:
	defines();

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

	const QColor &get_minor_nation_color() const
	{
		return this->minor_nation_color;
	}

	const QColor &get_country_border_color() const
	{
		return this->country_border_color;
	}

	const std::filesystem::path &get_default_settlement_image_filepath() const
	{
		return this->default_settlement_image_filepath;
	}

	void set_default_settlement_image_filepath(const std::filesystem::path &filepath);

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

signals:
	void changed();
	void scaled_tile_size_changed();

private:
	QSize tile_size = QSize(64, 64);
	terrain_type *default_base_terrain = nullptr;
	terrain_type *unexplored_terrain = nullptr;
	terrain_type *default_province_terrain = nullptr;
	terrain_type *default_water_zone_terrain = nullptr;
	QColor minor_nation_color;
	QColor country_border_color;
	std::filesystem::path default_settlement_image_filepath;
	std::filesystem::path default_menu_background_filepath;
	int min_diplomatic_map_tile_scale = 2;
};

}
