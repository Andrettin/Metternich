#pragma once

#include "map/province_container.h"
#include "map/terrain_type_container.h"

namespace metternich {

class country;
class region;

class map_generator final
{
private:
	static constexpr int max_latitude = 1000;
	static constexpr int temperature_level = 60;
	static constexpr int max_tile_value = 1000;

	static constexpr int tropical_threshold = std::min(map_generator::max_latitude * 9 / 10, map_generator::max_latitude * (143 * 7 - map_generator::temperature_level * 10) / 700);
	static constexpr int temperate_threshold = std::max(0, map_generator::max_latitude * (60 * 7 - map_generator::temperature_level * 6) / 700);

	static constexpr int min_land_elevation = 500;
	static constexpr int min_hill_elevation = 800;
	static constexpr int min_mountain_elevation = 900;

	static constexpr int min_forest_forestation = 900;

	enum class elevation_type {
		none,
		water,
		flatlands,
		hills,
		mountains
	};

	enum class climate_type {
		none,
		tropical,
		temperate,
		cold,
		frozen
	};

	enum class forestation_type {
		none,
		forest
	};

public:
	explicit map_generator(const QSize &size) : size(size)
	{
	}

	const QSize &get_size() const
	{
		return this->size;
	}

	int get_width() const
	{
		return this->get_size().width();
	}

	int get_height() const
	{
		return this->get_size().height();
	}

	void generate();

private:
	void generate_terrain();
	void generate_elevation();
	void generate_forestation();
	std::vector<QPoint> generate_tile_value_seeds(std::vector<int> &tile_values, const int seed_divisor);
	void expand_tile_value_seeds(const std::vector<QPoint> &base_seeds, std::vector<int> &tile_values);
	void generate_climate(const bool real);
	void adjust_tile_values(std::vector<int> &tile_values, const int min_value, const int max_value);

	void generate_provinces();
	std::vector<QPoint> generate_province_seeds(const size_t seed_count);
	void expand_province_seeds(const std::vector<QPoint> &base_seeds);

	void generate_countries();
	bool generate_ocean(const region *ocean);
	bool generate_country(const country *country);
	std::vector<const province *> generate_province_group(const std::vector<const province *> &potential_provinces, const int max_provinces, const province *capital_province);
	bool can_assign_province_to_province_index(const province *province, const int province_index) const;

	void generate_sites();

	elevation_type get_tile_elevation_type(const QPoint &tile_pos) const;

	bool is_tile_water(const QPoint &tile_pos) const
	{
		return this->get_tile_elevation_type(tile_pos) == elevation_type::water;
	}

	forestation_type get_tile_forestation_type(const QPoint &tile_pos) const;

	int get_tile_latitude(const QPoint &tile_pos) const
	{
		int latitude = tile_pos.y();

		const int half_width = this->get_width() / 2;

		if (latitude >= half_width) {
			latitude -= half_width;
		} else {
			latitude -= half_width - 1;
		}

		latitude *= -1;
		latitude *= map_generator::max_latitude;
		latitude /= half_width - 1;

		return latitude;
	}

	int get_tile_colatitude(const QPoint &tile_pos) const
	{
		const int abs_latitude = std::abs(this->get_tile_latitude(tile_pos));
		return map_generator::max_latitude - abs_latitude;
	}

private:
	QSize size = QSize(0, 0);
	int cold_threshold = 0;
	int province_count = 0;
	std::vector<QPoint> province_seeds;
	std::vector<std::vector<QPoint>> province_tiles;
	std::vector<terrain_type_map<std::vector<QPoint>>> province_tiles_by_terrain;
	std::vector<terrain_type_map<std::vector<QPoint>>> province_near_water_tiles_by_terrain;
	std::vector<int> tile_provinces;
	std::vector<int> tile_elevations;
	std::vector<int> tile_forestations;
	std::vector<climate_type> tile_climates;
	std::map<int, std::set<int>> province_border_provinces;
	province_set generated_provinces;
	std::map<int, const province *> provinces_by_index;
	province_map<const country *> province_owners;
};

}
