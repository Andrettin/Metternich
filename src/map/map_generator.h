#pragma once

#include "map/province_container.h"
#include "map/terrain_type_container.h"

namespace metternich {

class country;
class era;
class region;
enum class elevation_type;
enum class forestation_type;
enum class moisture_type;
enum class temperature_type;

class map_generator final
{
private:
	static constexpr int max_tile_value = 1000;

	static constexpr int min_land_elevation = 500;
	static constexpr int min_hill_elevation = 800;
	static constexpr int min_mountain_elevation = 900;

	static constexpr int min_temperate_temperature = 100;
	static constexpr int min_tropical_temperature = 750;

	static constexpr int min_semi_arid_moisture = 100;
	static constexpr int min_dry_moisture = 300;
	static constexpr int min_moist_moisture = 600;
	static constexpr int min_wet_moisture = 900;

	static constexpr int min_forest_forestation = 900;

public:
	explicit map_generator(const QSize &size, const metternich::era *era)
		: size(size), era(era)
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
	void generate_moisture();
	void generate_forestation();
	std::vector<QPoint> generate_tile_value_seeds(std::vector<int> &tile_values, const int seed_divisor);
	void expand_tile_value_seeds(const std::vector<QPoint> &base_seeds, std::vector<int> &tile_values, const int max_decrease);

	void generate_provinces();
	std::vector<QPoint> generate_province_seeds(const size_t seed_count);
	void expand_province_seeds(const std::vector<QPoint> &base_seeds);

	void generate_countries();
	bool generate_ocean(const region *ocean);
	bool generate_country(const country *country);
	std::vector<const province *> generate_province_group(const std::vector<const province *> &potential_provinces, const int max_provinces, const province *capital_province);
	int generate_province(const province *province, std::vector<int> &group_province_indexes);
	bool can_assign_province_to_province_index(const province *province, const int province_index) const;

	void generate_sites();

	elevation_type get_tile_elevation_type(const QPoint &tile_pos) const;
	bool is_tile_water(const QPoint &tile_pos) const;

	int get_tile_temperature(const QPoint &tile_pos) const;
	temperature_type get_tile_temperature_type(const QPoint &tile_pos) const;
	moisture_type get_tile_moisture_type(const QPoint &tile_pos) const;
	forestation_type get_tile_forestation_type(const QPoint &tile_pos) const;

	int get_tile_latitude(const QPoint &tile_pos) const
	{
		int latitude = tile_pos.y();

		const int half_height = this->get_height() / 2;

		if (latitude >= half_height) {
			latitude -= half_height;
		} else {
			latitude -= half_height - 1;
		}

		latitude *= -1;
		latitude *= map_generator::max_tile_value;
		latitude /= half_height - 1;

		return latitude;
	}

	int get_tile_colatitude(const QPoint &tile_pos) const
	{
		const int abs_latitude = std::abs(this->get_tile_latitude(tile_pos));
		return map_generator::max_tile_value - abs_latitude;
	}

private:
	QSize size = QSize(0, 0);
	const metternich::era *era = nullptr;
	int province_count = 0;
	std::vector<QPoint> province_seeds;
	std::vector<std::vector<QPoint>> province_tiles;
	std::vector<terrain_type_map<std::vector<QPoint>>> province_tiles_by_terrain;
	std::vector<terrain_type_map<std::vector<QPoint>>> province_near_water_tiles_by_terrain;
	std::set<int> sea_zones;
	std::set<int> lakes;
	std::vector<int> tile_provinces;
	std::vector<int> tile_elevations;
	std::vector<int> tile_moistures;
	std::vector<int> tile_forestations;
	std::map<int, std::set<int>> province_border_provinces;
	province_set generated_provinces;
	std::map<int, const province *> provinces_by_index;
	province_map<const country *> province_owners;
};

}
