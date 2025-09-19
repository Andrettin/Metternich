#pragma once

#include "map/province_container.h"
#include "map/terrain_type_container.h"

namespace metternich {

class domain;
class map_template;
class region;
enum class elevation_type;
enum class forestation_type;
enum class moisture_type;
enum class temperature_type;

class map_generator final
{
private:
	struct zone final
	{
		explicit zone(const QPoint &seed) : seed(seed)
		{
			this->tiles.push_back(seed);
		}

		QPoint seed;
		std::vector<QPoint> tiles;
		terrain_type_map<std::vector<QPoint>> tiles_by_terrain;
		terrain_type_map<std::vector<QPoint>> near_water_tiles_by_terrain;
		terrain_type_map<std::vector<QPoint>> coastal_tiles_by_terrain;
		std::set<int> border_zones;
		bool removed = false;
	};

	static constexpr int max_colatitude = 1000;
	static constexpr int max_elevation = 1000;

	static constexpr int max_tile_value = 1000;

	static constexpr int min_semi_arid_moisture = 100;
	static constexpr int min_dry_moisture = 300;
	static constexpr int min_moist_moisture = 600;
	static constexpr int min_wet_moisture = 900;

	static constexpr int min_forest_forestation = 900;

public:
	static void adjust_values(std::vector<int> &values, const int target_max_value);

	explicit map_generator(const metternich::map_template *map_template)
		: map_template(map_template)
	{
	}

	const metternich::map_template *get_map_template() const
	{
		return this->map_template;
	}

	const QSize &get_size() const;

	int get_width() const
	{
		return this->get_size().width();
	}

	int get_height() const
	{
		return this->get_size().height();
	}

	int get_area() const
	{
		return this->get_width() * this->get_height();
	}

	int get_tile_count() const
	{
		return this->get_width() * this->get_height();
	}

	void generate();

private:
	void initialize_temperature_levels();

	void generate_terrain();
	void generate_world_terrain();

	void generate_elevation();
	void generate_pseudofractal_elevation(const int additional_initial_block_count);
	void generate_pseudofractal_tile_rect_elevation(const int step, const QRect &tile_rect);
	void set_pseudofractal_elevation_midpoints(const int x, const int y, const int value);

	void generate_temperature();
	void generate_moisture();
	void generate_forestation();
	std::vector<QPoint> generate_tile_value_seeds(std::vector<int> &tile_values, const int seed_divisor);
	void expand_tile_value_seeds(const std::vector<QPoint> &base_seeds, std::vector<int> &tile_values, const int max_decrease);

	void generate_zones();
	std::vector<QPoint> generate_zone_seeds(const size_t seed_count);
	void expand_zone_seeds(const std::vector<QPoint> &base_seeds);
	void consolidate_water_zones();
	int choose_sea_zone_for_removal(const std::set<int> &remaining_sea_zones, const std::vector<int> &sea_zones_to_remove, std::vector<std::vector<int>> &distance_cache) const;
	void remove_zone(const int zone_index);

	void generate_provinces();
	void generate_countries();
	void generate_countries_from_provinces(const std::vector<const province *> &provinces);
	bool generate_ocean(const region *ocean);
	bool generate_country(const domain *domain, const std::vector<const province *> &country_provinces);
	void generate_star_systems();
	std::vector<const province *> generate_province_group(const std::vector<const province *> &potential_provinces, const province *capital_province);
	int generate_province(const province *province, std::vector<int> &group_zone_indexes);
	bool can_assign_province_to_zone_index(const province *province, const int zone_index) const;
	static int get_province_distance_multiplier_to(const province *province, const metternich::province *other_province);

	void generate_sites();
	void generate_site(const site *site, const zone &zone);

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
		latitude *= map_generator::max_colatitude;
		latitude /= half_height - 1;

		return latitude;
	}

	int get_tile_colatitude(const QPoint &tile_pos) const
	{
		const int abs_latitude = std::abs(this->get_tile_latitude(tile_pos));
		return map_generator::max_colatitude - abs_latitude;
	}

	int get_min_land_elevation() const;
	int get_min_hill_elevation() const;
	int get_min_mountain_elevation() const;

	int get_tropical_level() const;
	int get_temperate_level() const;

	int get_ice_base_level() const
	{
		return this->ice_base_level;
	}

	bool is_tile_near_edge(const QPoint &tile_pos) const;

	int get_sqrt_size() const;

	bool is_x_wrap_enabled() const
	{
		return false;
	}

	bool is_y_wrap_enabled() const
	{
		return false;
	}

private:
	const metternich::map_template *map_template = nullptr;
	int ice_base_level = 0;
	int zone_count = 0;
	std::vector<zone> zones;
	std::set<int> sea_zones;
	std::set<int> lakes;
	std::vector<int> tile_zones;
	std::vector<int> tile_elevations;
	std::vector<int> tile_temperatures;
	std::vector<int> tile_moistures;
	std::vector<int> tile_forestations;
	province_set generated_provinces;
	std::map<int, const province *> provinces_by_zone_index;
};

}
