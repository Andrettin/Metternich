#pragma once

#include "map/province_container.h"

namespace metternich {

class country;

class map_generator final
{
private:
	static constexpr int max_latitude = 1000;
	static constexpr int max_height = 1000;
	static constexpr int temperature_level = 60;
	static constexpr int land_percent = 30;
	static constexpr int pole_flattening = 0;

	static constexpr int tropical_threshold = std::min(map_generator::max_latitude * 9 / 10, map_generator::max_latitude * (143 * 7 - map_generator::temperature_level * 10) / 700);
	static constexpr int temperate_threshold = std::max(0, map_generator::max_latitude * (60 * 7 - map_generator::temperature_level * 6) / 700);

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
	void generate_climate(const bool real);
	void generate_heightmap();
	void generate_rectangle_height(const int step, const QRect &rect);
	void set_height_midpoints(const QPoint &tile_pos, const int value);
	void generate_land();
	void normalize_pole_heights();
	void adjust_tile_values(std::vector<int> &tile_values, const int min_value, const int max_value);

	void generate_provinces();
	std::vector<QPoint> generate_province_seeds(const size_t seed_count);
	void expand_province_seeds(const std::vector<QPoint> &base_seeds);
	bool generate_country(const country *country);
	std::vector<const province *> generate_province_group(const std::vector<const province *> &potential_provinces, const int max_provinces, const province *capital_province);

	QPoint get_wrapped_tile_pos(const QPoint &tile_pos) const
	{
		return QPoint(tile_pos.x() % this->get_width(), tile_pos.y());
	}

	bool is_tile_on_edge(const QPoint &tile_pos) const
	{
		//x wraps around for the purposes of map generation, so it doesn't affect this

		return tile_pos.y() == 0 || tile_pos.y() == (this->get_height() - 1);
	}

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

	int get_tile_pole_height_factor(const QPoint &tile_pos) const
	{
		int factor = 100;

		if (this->is_tile_on_edge(tile_pos)) {
			factor = 100 - map_generator::pole_flattening;
		} else if (map_generator::pole_flattening > 0) {
			factor = 100 - ((100 - (this->get_tile_colatitude(tile_pos) / (this->cold_threshold * 5 / 2 / 2))) * map_generator::pole_flattening);
		}

		if (this->get_tile_colatitude(tile_pos) >= this->cold_threshold) {
			factor = std::min(factor, 10);
		}

		return factor;
	}

private:
	QSize size = QSize(0, 0);
	int cold_threshold = 0;
	int province_count = 0;
	std::vector<QPoint> province_seeds;
	std::vector<int> tile_provinces;
	std::vector<climate_type> tile_climates;
	std::vector<int> tile_heights;
	std::map<int, std::set<int>> province_border_provinces;
	province_set generated_provinces;
	std::map<int, const province *> provinces_by_index;
	province_map<const country *> province_owners;
};

}
