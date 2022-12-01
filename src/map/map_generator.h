#pragma once

#include "map/province_container.h"

namespace metternich {

class country;
class region;

class map_generator final
{
private:
	static constexpr int max_latitude = 1000;
	static constexpr int temperature_level = 60;
	static constexpr int min_land_percent = 25;
	static constexpr int max_land_percent = 50;

	static constexpr int tropical_threshold = std::min(map_generator::max_latitude * 9 / 10, map_generator::max_latitude * (143 * 7 - map_generator::temperature_level * 10) / 700);
	static constexpr int temperate_threshold = std::max(0, map_generator::max_latitude * (60 * 7 - map_generator::temperature_level * 6) / 700);

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
	std::vector<QPoint> expand_elevation_seeds(const std::vector<QPoint> &base_seeds, const elevation_type elevation_type, const int expansion_chance, const int max_tiles);
	void generate_climate(const bool real);
	void adjust_tile_values(std::vector<int> &tile_values, const int min_value, const int max_value);

	void generate_provinces();
	std::vector<QPoint> generate_province_seeds(const size_t seed_count);
	void expand_province_seeds(const std::vector<QPoint> &base_seeds);
	bool generate_ocean(const region *ocean);
	bool generate_country(const country *country);
	std::vector<const province *> generate_province_group(const std::vector<const province *> &potential_provinces, const int max_provinces, const province *capital_province);

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
	std::vector<int> tile_provinces;
	std::vector<elevation_type> tile_elevation_types;
	std::vector<climate_type> tile_climates;
	std::map<int, std::set<int>> province_border_provinces;
	province_set generated_provinces;
	std::map<int, const province *> provinces_by_index;
	province_map<const country *> province_owners;
};

}
