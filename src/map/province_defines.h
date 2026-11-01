#pragma once

#include "database/defines_base.h"
#include "economy/commodity_container.h"
#include "util/dice.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("map/terrain_type.h")

namespace metternich {

class terrain_type;
enum class province_taxation_type;

class province_defines final : public defines_base, public singleton<province_defines>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::terrain_type *default_province_terrain MEMBER default_province_terrain  READ get_default_province_terrain NOTIFY changed)
	Q_PROPERTY(const metternich::terrain_type *default_water_zone_terrain MEMBER default_water_zone_terrain  READ get_default_water_zone_terrain NOTIFY changed)
	Q_PROPERTY(int province_map_tile_scale MEMBER province_map_tile_scale READ get_province_map_tile_scale NOTIFY changed)
	Q_PROPERTY(int province_population_capacity_level_per_level MEMBER province_population_capacity_level_per_level READ get_province_population_capacity_level_per_level NOTIFY changed)

public:
	province_defines();
	~province_defines();

	virtual std::string_view get_file_name() const override
	{
		return "province_defines.txt";
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const terrain_type *get_default_province_terrain() const
	{
		return this->default_province_terrain;
	}

	const terrain_type *get_default_water_zone_terrain() const
	{
		return this->default_water_zone_terrain;
	}

	int get_province_map_tile_scale() const
	{
		return this->province_map_tile_scale;
	}

	int get_province_population_capacity_level_per_level() const;
	int get_province_level_for_population(const int64_t population) const;

	const commodity_map<int> &get_province_level_commodity_costs_per_level() const
	{
		return this->province_level_commodity_costs_per_level;
	}

	const dice &get_province_taxation_for_level(const province_taxation_type taxation_type, const int level) const;

signals:
	void changed();

private:
	const terrain_type *default_province_terrain = nullptr;
	const terrain_type *default_water_zone_terrain = nullptr;
	int province_map_tile_scale = 2;
	int province_population_capacity_level_per_level = 0;
	commodity_map<int> province_level_commodity_costs_per_level;
	std::map<province_taxation_type, std::map<int, dice>> province_taxation_per_level;
};

}
