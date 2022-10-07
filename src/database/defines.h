#pragma once

#include "database/defines_base.h"
#include "util/singleton.h"

namespace metternich {

class gsml_data;
class gsml_property;
class terrain_type;

class defines final : public defines_base, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(metternich::terrain_type* default_province_terrain MEMBER default_province_terrain)
	Q_PROPERTY(metternich::terrain_type* default_water_zone_terrain MEMBER default_water_zone_terrain)
	Q_PROPERTY(QString default_menu_background_filepath READ get_default_menu_background_filepath_qstring NOTIFY changed)

public:
	const terrain_type *get_default_province_terrain() const
	{
		return this->default_province_terrain;
	}

	const terrain_type *get_default_water_zone_terrain() const
	{
		return this->default_water_zone_terrain;
	}

	QString get_default_menu_background_filepath_qstring() const;
	void set_default_menu_background_filepath(const std::filesystem::path &filepath);

	Q_INVOKABLE void set_default_menu_background_filepath(const std::string &filepath)
	{
		this->set_default_menu_background_filepath(std::filesystem::path(filepath));
	}

signals:
	void changed();

private:
	terrain_type *default_province_terrain = nullptr;
	terrain_type *default_water_zone_terrain = nullptr;
	std::filesystem::path default_menu_background_filepath;
};

}
