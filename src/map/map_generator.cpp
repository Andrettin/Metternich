#include "metternich.h"

#include "map/map_generator.h"

#include "country/country.h"
#include "database/defines.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace metternich {

void map_generator::generate()
{
	map *map = map::get();
	map->clear();
	map->set_size(this->get_size());
	map->create_tiles();

	const int tile_count = this->get_width() * this->get_height();
	this->tile_provinces.resize(tile_count, -1);
	this->tile_climates.resize(tile_count, climate_type::none);

	this->generate_terrain();
	this->generate_provinces();

	map->initialize();

	for (const auto &[province, country] : this->province_owners) {
		province->get_game_data()->set_owner(country);
	}
}

void map_generator::generate_terrain()
{
	const int tile_count = this->get_width() * this->get_height();
	const int sqrt_tile_count = std::max(1, number::sqrt(tile_count / 1000));
	this->cold_threshold = ((std::max(0, 100 * map_generator::temperate_threshold / 3 - 1 * map_generator::max_latitude) + 1 * map_generator::max_latitude * sqrt_tile_count) / (100 * sqrt_tile_count)) * 2;

	this->generate_climate(false);
	this->generate_heightmap();
	this->generate_land();
}

void map_generator::generate_climate(const bool real)
{
	std::vector<int> tile_climate_values;
	tile_climate_values.resize(this->tile_climates.size(), -1);

	if (real) {
		//FIXME: implement
	} else {
		for (int x = 0; x < this->get_width(); ++x) {
			for (int y = 0; y < this->get_height(); ++y) {
				const QPoint tile_pos(x, y);
				const int tile_index = point::to_index(tile_pos, this->get_width());
				tile_climate_values[tile_index] = this->get_tile_colatitude(tile_pos);
			}
		}
	}

	this->adjust_tile_values(tile_climate_values, 0, map_generator::max_latitude);

	for (size_t i = 0; i < tile_climate_values.size(); ++i) {
		const int climate_value = tile_climate_values[i];
		if (climate_value >= map_generator::tropical_threshold) {
			this->tile_climates[i] = climate_type::tropical;
		} else if (climate_value >= map_generator::temperate_threshold) {
			this->tile_climates[i] = climate_type::temperate;
		} else if (climate_value >= cold_threshold) {
			this->tile_climates[i] = climate_type::cold;
		} else {
			this->tile_climates[i] = climate_type::frozen;
		}
	}
}

void map_generator::generate_heightmap()
{
	static constexpr bool x_wrap = true;
	static constexpr bool y_wrap = false;

	const int x_div = 6;
	const int y_div = 6;

	int x_div_2 = x_div + (x_wrap ? 0 : 1);
	int y_div_2 = y_div + (y_wrap ? 0 : 1);

	int x_max = this->get_width() - (x_wrap ? 0 : 1);
	int y_max = this->get_height() - (y_wrap ? 0 : 1);

	int x_current = 0;
	int y_current = 0;

	int step = this->get_width() + this->get_height();

	int avoid_edge = (100 - map_generator::land_percent) * step / 100 + step / 3;

	const int tile_count = this->get_width() * this->get_height();
	this->tile_heights.resize(tile_count, 0);

	const map *map = map::get();

	for (x_current = 0; x_current < x_div_2; ++x_current) {
		for (y_current = 0; y_current < y_div_2; ++y_current) {
			const int x = x_current * x_max / x_div;
			const int y = y_current * y_max / y_div;

			const QPoint tile_pos = this->get_wrapped_tile_pos(QPoint(x, y));
			assert_throw(map->contains(tile_pos));

			int &tile_height = this->tile_heights[point::to_index(tile_pos, this->get_width())];
			tile_height = random::get()->generate(2 * step) - (2 * step) / 2;

			if (this->is_tile_on_edge(tile_pos)) {
				tile_height -= avoid_edge;
			}

			if (this->get_tile_colatitude(tile_pos) <= (this->cold_threshold / 4) && map_generator::pole_flattening > 0) {
				tile_height -= random::get()->generate(avoid_edge * map_generator::pole_flattening / 100);
			}
		}
	}

	for (x_current = 0; x_current < x_div; ++x_current) {
		for (y_current = 0; y_current < y_div; ++y_current) {
			const QPoint top_left(x_current * x_max / x_div, y_current * y_max / y_div);
			const QPoint bottom_right((x_current + 1) * x_max / x_div, (y_current + 1) * y_max / y_div);
			const QRect rect(top_left, bottom_right - QPoint(1, 1));
			this->generate_rectangle_height(step, rect);
		}
	}

	for (int &tile_height : this->tile_heights) {
		tile_height = 8 * tile_height + random::get()->generate(4) - 2;
	}

	this->adjust_tile_values(this->tile_heights, 0, map_generator::max_height);
}

void map_generator::generate_rectangle_height(const int step, const QRect &rect)
{
	std::array<std::array<int, 2>, 2> val{};
	int x1_wrap = rect.right() + 1;
	int y1_wrap = rect.bottom() + 1;

	if (rect.width() <= 0 || rect.height() <= 0) {
		return;
	}

	if (rect.width() == 1 && rect.height() == 1) {
		return;
	}

	if (rect.right() == (this->get_width() - 1)) {
		x1_wrap = 0;
	}

	if (rect.bottom() == (this->get_height() - 1)) {
		y1_wrap = 0;
	}

	val[0][0] = this->tile_heights[point::to_index(this->get_wrapped_tile_pos(rect.topLeft()), this->get_width())];
	val[0][1] = this->tile_heights[point::to_index(this->get_wrapped_tile_pos(QPoint(rect.left(), y1_wrap)), this->get_width())];
	val[1][0] = this->tile_heights[point::to_index(this->get_wrapped_tile_pos(QPoint(x1_wrap, rect.top())), this->get_width())];
	val[1][1] = this->tile_heights[point::to_index(this->get_wrapped_tile_pos(QPoint(x1_wrap, y1_wrap)), this->get_width())];

	const int center_x = (rect.left() + rect.right() + 1) / 2;
	const int center_y = (rect.top() + rect.bottom() + 1) / 2;
	const QPoint center_pos(center_x, center_y);

	this->set_height_midpoints(QPoint(center_x, rect.top()), (val[0][0] + val[1][0]) / 2 + random::get()->generate(step) - step / 2);
	this->set_height_midpoints(QPoint(center_x, y1_wrap), (val[0][1] + val[1][1]) / 2 + random::get()->generate(step) - step / 2);
	this->set_height_midpoints(QPoint(rect.left(), center_y), (val[0][0] + val[0][1]) / 2 + random::get()->generate(step) - step / 2);
	this->set_height_midpoints(QPoint(x1_wrap, center_y), (val[1][0] + val[1][1]) / 2 + random::get()->generate(step) - step / 2);

	this->set_height_midpoints(center_pos, ((val[0][0] + val[0][1] + val[1][0] + val[1][1]) / 4 + random::get()->generate(step) - step / 2));

	this->generate_rectangle_height(2 * step / 3, QRect(rect.topLeft(), center_pos - QPoint(1, 1)));
	this->generate_rectangle_height(2 * step / 3, QRect(QPoint(rect.left(), center_y), QPoint(center_x, rect.bottom()) - QPoint(1, 1)));
	this->generate_rectangle_height(2 * step / 3, QRect(QPoint(center_x, rect.top()), QPoint(rect.right(), center_y) - QPoint(1, 1)));
	this->generate_rectangle_height(2 * step / 3, QRect(center_pos, rect.bottomRight() - QPoint(1, 1)));
}

void map_generator::set_height_midpoints(const QPoint &tile_pos, const int value)
{
	int &tile_height = this->tile_heights[point::to_index(tile_pos, this->get_width())];

	if (this->get_tile_colatitude(tile_pos) <= (this->cold_threshold / 4)) {
		tile_height = value * (100 - map_generator::pole_flattening) / 100;
	} else if (!this->is_tile_on_edge(tile_pos) && tile_height == 0) {
		tile_height = value;
	}
}

void map_generator::generate_land()
{
	this->normalize_pole_heights();

	map *map = map::get();

	const terrain_type *land_terrain = defines::get()->get_default_province_terrain();
	const terrain_type *water_terrain = defines::get()->get_default_water_zone_terrain();

	const int shore_height_level = (map_generator::max_height * (100 - map_generator::land_percent)) / 100;

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const int tile_index = point::to_index(tile_pos, this->get_width());
			const int tile_height = this->tile_heights[tile_index];
			if (tile_height < shore_height_level) {
				map->set_tile_terrain(tile_pos, water_terrain);
			} else {
				map->set_tile_terrain(tile_pos, land_terrain);
			}
		}
	}
}

void map_generator::normalize_pole_heights()
{
	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const int tile_index = point::to_index(tile_pos, this->get_width());

			if (this->get_tile_colatitude(tile_pos) <= this->cold_threshold * 5 / 2 / 2) {
				this->tile_heights[tile_index] *= this->get_tile_pole_height_factor(tile_pos);
				this->tile_heights[tile_index] /= 100;
			} else if (this->is_tile_on_edge(tile_pos)) {
				this->tile_heights[tile_index] = 0;
			}
		}
	}
}

void map_generator::adjust_tile_values(std::vector<int> &tile_values, const int min_value, const int max_value)
{
	const int delta = max_value - min_value;

	int min_old_value = 0;
	int max_old_value = 0;

	bool first = true;

	for (const int old_value : tile_values) {
		if (first) {
			first = false;
			min_old_value = old_value;
			max_old_value = old_value;
		} else {
			max_old_value = std::max(max_old_value, old_value);
			min_old_value = std::min(min_old_value, old_value);
		}
	}

	const int old_size = 1 + max_old_value - min_old_value;

	std::vector<int> frequencies;
	frequencies.resize(old_size, 0);

	for (int &tile_value : tile_values) {
		tile_value -= min_old_value;
		++frequencies[tile_value];
	}

	int count = 0;

	for (int &frequency : frequencies) {
		count += frequency;
		frequency = min_value + (count * delta) / static_cast<int>(tile_values.size());
	}

	for (int &tile_value : tile_values) {
		tile_value = frequencies[tile_value];
	}
}

void map_generator::generate_provinces()
{
	const int map_area = this->get_width() * this->get_height();

	this->province_count = map_area / 256;

	this->province_seeds = this->generate_province_seeds(static_cast<size_t>(this->province_count));
	this->expand_province_seeds(this->province_seeds);

	std::vector<const country *> potential_powers;
	std::vector<const country *> potential_minor_nations;

	for (const country *country : country::get_all()) {
		if (country->get_provinces().empty()) {
			continue;
		}

		if (country->is_great_power()) {
			potential_powers.push_back(country);
		} else {
			potential_minor_nations.push_back(country);
		}
	}

	vector::shuffle(potential_powers);
	vector::shuffle(potential_minor_nations);

	const int max_powers = map_area / 1024;
	int power_count = 0;

	for (const country *country : potential_powers) {
		if (static_cast<int>(this->generated_provinces.size()) >= this->province_count) {
			break;
		}

		if (this->generate_country(country)) {
			++power_count;
			if (power_count == max_powers) {
				break;
			}
		}
	}

	for (const country *country : potential_minor_nations) {
		if (static_cast<int>(this->generated_provinces.size()) >= this->province_count) {
			break;
		}

		this->generate_country(country);
	}

	assert_throw(static_cast<int>(this->generated_provinces.size()) == this->province_count);

	map *map = map::get();

	for (size_t i = 0; i < this->tile_provinces.size(); ++i) {
		const int province_index = this->tile_provinces[i];
		assert_throw(province_index >= 0);

		const province *province = this->provinces_by_index.find(province_index)->second;
		assert_throw(province != nullptr);
		map->set_tile_province(point::from_index(static_cast<int>(i), this->get_width()), province);
	}
}

std::vector<QPoint> map_generator::generate_province_seeds(const size_t seed_count)
{
	static constexpr int min_province_seed_distance = 8;

	std::vector<QPoint> potential_positions;
	potential_positions.reserve(this->tile_provinces.size());

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			potential_positions.emplace_back(x, y);
		}
	}

	std::vector<QPoint> province_seeds;
	province_seeds.reserve(seed_count);

	for (size_t i = 0; i < seed_count; ++i) {
		while (!potential_positions.empty()) {
			QPoint random_pos = vector::take_random(potential_positions);

			bool valid_location = true;
			for (const QPoint &seed : province_seeds) {
				//when generating province seeds, make them have a certain distance between each other
				const int distance = point::distance_to(random_pos, seed);

				if (distance < min_province_seed_distance) {
					valid_location = false;
					break;
				}
			}

			if (valid_location) {
				province_seeds.push_back(std::move(random_pos));
				break;
			}
		}
	}

	assert_throw(province_seeds.size() == seed_count);

	for (size_t i = 0; i < province_seeds.size(); ++i) {
		const QPoint seed_pos = province_seeds.at(i);
		this->tile_provinces[point::to_index(seed_pos, this->get_width())] = static_cast<int>(i);
	}

	return province_seeds;
}

void map_generator::expand_province_seeds(const std::vector<QPoint> &base_seeds)
{
	const map *map = map::get();
	const QSize &map_size = this->get_size();

	std::vector<QPoint> seeds = base_seeds;

	while (!seeds.empty()) {
		QPoint seed_pos = vector::take_random(seeds);
		const int province_index = this->tile_provinces[point::to_index(seed_pos, map_size)];

		std::vector<QPoint> adjacent_positions;

		point::for_each_cardinally_adjacent(seed_pos, [&](QPoint &&adjacent_pos) {
			if (!map->contains(adjacent_pos)) {
				return;
			}

			const int adjacent_province_index = this->tile_provinces[point::to_index(adjacent_pos, map_size)];
			if (adjacent_province_index != -1) {
				//the adjacent tile must not have a province assigned yet
				this->province_border_provinces[province_index].insert(adjacent_province_index);
				this->province_border_provinces[adjacent_province_index].insert(province_index);
				return;
			}

			adjacent_positions.push_back(std::move(adjacent_pos));
		});

		if (adjacent_positions.empty()) {
			continue;
		}

		if (adjacent_positions.size() > 1) {
			//push the seed back again for another try, since it may be able to generate further in the future
			seeds.push_back(std::move(seed_pos));
		}

		QPoint adjacent_pos = vector::take_random(adjacent_positions);
		this->tile_provinces[point::to_index(adjacent_pos, map_size)] = province_index;

		seeds.push_back(std::move(adjacent_pos));
	}
}

bool map_generator::generate_country(const country *country)
{
	if (this->generated_provinces.contains(country->get_capital_province())) {
		return false;
	}

	static constexpr int max_country_provinces = 8;
	int generated_province_count = 0;

	std::vector<int> country_province_indexes;

	for (const province *province : country->get_provinces()) {
		if (this->generated_provinces.contains(province)) {
			continue;
		}

		int province_index = -1;

		if (generated_province_count == 0) {
			//first province
			std::vector<int> best_province_indexes;
			int best_distance = 0;

			//get the provinces which are as far away from other powers as possible
			for (int i = 0; i < this->province_count; ++i) {
				if (this->provinces_by_index.contains(i)) {
					//already generated
					continue;
				}

				const QPoint &province_seed = this->province_seeds.at(i);

				int distance = std::numeric_limits<int>::max();

				for (const auto &[other_province_index, other_province] : this->provinces_by_index) {
					const QPoint &other_province_seed = this->province_seeds.at(i);
					distance = std::min(distance, point::distance_to(province_seed, other_province_seed));
				}

				if (distance > best_distance) {
					best_province_indexes.clear();
					best_distance = distance;
				}
					
				if (distance == best_distance) {
					best_province_indexes.push_back(i);
				}
			}

			if (!best_province_indexes.empty()) {
				province_index = vector::get_random(best_province_indexes);
			}
		} else {
			//pick a province bordering one of the country's existing provinces
			std::vector<int> potential_province_indexes;

			for (const int country_province_index : country_province_indexes) {
				for (const int border_province_index : this->province_border_provinces[country_province_index]) {
					if (vector::contains(potential_province_indexes, border_province_index)) {
						continue;
					}

					if (this->provinces_by_index.contains(border_province_index)) {
						//already generated
						continue;
					}

					potential_province_indexes.push_back(border_province_index);
				}
			}

			if (!potential_province_indexes.empty()) {
				province_index = vector::get_random(potential_province_indexes);
			}
		}

		if (province_index == -1) {
			if (province == country->get_capital_province()) {
				return false;
			}

			continue;
		}

		this->provinces_by_index[province_index] = province;
		country_province_indexes.push_back(province_index);

		this->generated_provinces.insert(province);
		++generated_province_count;

		this->province_owners[province] = country;

		if (generated_province_count == max_country_provinces) {
			break;
		}

		if (static_cast<int>(this->generated_provinces.size()) == this->province_count) {
			break;
		}
	}

	return true;
}

}
