#include "metternich.h"

#include "map/map_generator.h"

#include "country/country.h"
#include "database/defines.h"
#include "economy/resource.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
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
	this->tile_elevations.resize(tile_count, -1);
	this->tile_forestations.resize(tile_count, -1);
	this->tile_climates.resize(tile_count, climate_type::none);

	this->generate_provinces();
	this->generate_terrain();
	this->generate_countries();

	//assign provinces
	for (size_t i = 0; i < this->tile_provinces.size(); ++i) {
		const int province_index = this->tile_provinces[i];
		assert_throw(province_index >= 0);

		const auto find_iterator = this->provinces_by_index.find(province_index);
		//assert_throw(find_iterator != this->provinces_by_index.end());

		if (find_iterator == this->provinces_by_index.end()) {
			continue;
		}

		const province *province = find_iterator->second;
		assert_throw(province != nullptr);
		map->set_tile_province(point::from_index(static_cast<int>(i), this->get_width()), province);
	}

	map->initialize();

	for (const auto &[province, country] : this->province_owners) {
		province->get_game_data()->set_owner(country);
	}
}

void map_generator::generate_terrain()
{
	//ensure edge provinces are water
	const QRect map_rect(QPoint(0, 0), this->get_size());
	rect::for_each_edge_point(map_rect, [&](const QPoint &tile_pos) {
		const int tile_index = point::to_index(tile_pos, this->get_width());
		const int province_index = this->tile_provinces[tile_index];
		const QPoint &province_seed = this->province_seeds.at(province_index);
		const int province_seed_index = point::to_index(province_seed, this->get_width());
		this->tile_elevations[tile_index] = 0;
		this->tile_elevations[province_seed_index] = 0;
	});

	this->generate_elevation();
	//this->generate_climate();
	this->generate_forestation();

	//assign terrain
	const terrain_type *land_terrain = defines::get()->get_default_province_terrain();
	const terrain_type *forest_terrain = terrain_type::get("conifer_forest");
	const terrain_type *hills_terrain = terrain_type::get("barren_hills");
	const terrain_type *mountains_terrain = terrain_type::get("mountains");
	const terrain_type *water_terrain = defines::get()->get_default_water_zone_terrain();

	map *map = map::get();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const int tile_index = point::to_index(tile_pos, this->get_width());

			int &elevation = this->tile_elevations[tile_index];
			const bool is_water = this->is_tile_water(tile_pos);

			const int province_index = this->tile_provinces[tile_index];
			const bool province_is_water = this->is_tile_water(this->province_seeds[province_index]);

			if (province_is_water && !is_water) {
				elevation = 0;
			} else if (!province_is_water && is_water) {
				elevation = map_generator::min_land_elevation;
			}

			const terrain_type *terrain = nullptr;

			const elevation_type elevation_type = this->get_tile_elevation_type(tile_pos);
			const forestation_type forestation_type = this->get_tile_forestation_type(tile_pos);

			switch (elevation_type) {
				case elevation_type::water:
					terrain = water_terrain;
					break;
				case elevation_type::flatlands:
					switch (forestation_type) {
					case forestation_type::none:
						terrain = land_terrain;
						break;
					case forestation_type::forest:
						terrain = forest_terrain;
						break;
					default:
						assert_throw(false);
					}
					break;
				case elevation_type::hills:
					terrain = hills_terrain;
					break;
				case elevation_type::mountains:
					terrain = mountains_terrain;
					break;
				default:
					assert_throw(false);
			}

			map->set_tile_terrain(tile_pos, terrain);
		}
	}
}

void map_generator::generate_elevation()
{
	const std::vector<QPoint> elevation_seeds = this->generate_tile_value_seeds(this->tile_elevations, 1024);
	this->expand_tile_value_seeds(elevation_seeds, this->tile_elevations);
}

void map_generator::generate_forestation()
{
	const std::vector<QPoint> seeds = this->generate_tile_value_seeds(this->tile_forestations, 256);
	this->expand_tile_value_seeds(seeds, this->tile_forestations);
}

std::vector<QPoint> map_generator::generate_tile_value_seeds(std::vector<int> &tile_values, const int seed_divisor)
{
	const int map_area = this->get_width() * this->get_height();
	const int seed_count = map_area / seed_divisor;

	std::vector<QPoint> potential_positions;
	potential_positions.reserve(tile_values.size());

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			potential_positions.emplace_back(x, y);
		}
	}

	std::vector<QPoint> seeds;
	seeds.reserve(seed_count);

	for (int i = 0; i < seed_count; ++i) {
		while (!potential_positions.empty()) {
			QPoint random_pos = vector::take_random(potential_positions);

			const int tile_index = point::to_index(random_pos, this->get_width());
			int &tile_value = tile_values[tile_index];
			if (tile_value != -1) {
				continue;
			}

			tile_value = map_generator::max_tile_value;

			seeds.push_back(std::move(random_pos));
			break;
		}
	}

	assert_throw(static_cast<int>(seeds.size()) == seed_count);

	return seeds;
}

void map_generator::expand_tile_value_seeds(const std::vector<QPoint> &base_seeds, std::vector<int> &tile_values)
{
	const map *map = map::get();

	std::vector<QPoint> seeds = base_seeds;

	while (!seeds.empty()) {
		const QPoint seed_pos = vector::take_random(seeds);

		const int tile_value = tile_values[point::to_index(seed_pos, this->get_width())];

		point::for_each_cardinally_adjacent(seed_pos, [&](QPoint &&adjacent_pos) {
			if (!map->contains(adjacent_pos)) {
				return;
			}

			int &adjacent_tile_value = tile_values[point::to_index(adjacent_pos, this->get_width())];
			if (adjacent_tile_value != -1) {
				return;
			}

			adjacent_tile_value = std::max(0, tile_value - random::get()->generate_in_range(0, 50));
			seeds.push_back(std::move(adjacent_pos));
		});
	}
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

	this->province_count = map_area / 128;

	this->province_seeds = this->generate_province_seeds(static_cast<size_t>(this->province_count));
	this->expand_province_seeds(this->province_seeds);
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
		const int tile_index = point::to_index(seed_pos, this->get_width());
		this->tile_provinces[tile_index] = static_cast<int>(i);
		this->province_tiles.push_back({ seed_pos });
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

		const int tile_index = point::to_index(seed_pos, map_size);
		const int province_index = this->tile_provinces[tile_index];

		std::vector<QPoint> adjacent_positions;

		point::for_each_cardinally_adjacent(seed_pos, [&](QPoint &&adjacent_pos) {
			if (!map->contains(adjacent_pos)) {
				return;
			}

			const int adjacent_tile_index = point::to_index(adjacent_pos, map_size);
			const int adjacent_province_index = this->tile_provinces[adjacent_tile_index];
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
		this->province_tiles[province_index].push_back(adjacent_pos);

		seeds.push_back(std::move(adjacent_pos));
	}

	//set tiles without provinces (e.g. water tiles which are enclaves in land provinces) to the most-neighbored province
	std::vector<QPoint> remaining_positions;

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			QPoint tile_pos(x, y);
			const int tile_index = point::to_index(tile_pos, this->get_width());

			if (this->tile_provinces[tile_index] != -1) {
				continue;
			}

			remaining_positions.push_back(std::move(tile_pos));
		}
	}

	while (!remaining_positions.empty()) {
		for (size_t i = 0; i < remaining_positions.size();) {
			const QPoint &tile_pos = remaining_positions.at(i);
			const int tile_index = point::to_index(tile_pos, this->get_width());

			std::map<int, int> province_neighbor_counts;

			point::for_each_cardinally_adjacent(tile_pos, [&](QPoint &&adjacent_pos) {
				if (!map->contains(adjacent_pos)) {
					return;
				}

				const int adjacent_tile_index = point::to_index(adjacent_pos, map_size);
				const int adjacent_province_index = this->tile_provinces[adjacent_tile_index];

				if (adjacent_province_index != -1) {
					province_neighbor_counts[adjacent_province_index]++;
				}
			});

			if (province_neighbor_counts.empty()) {
				//no adjacent province available (i.e. the tile is surrounded by others which also have no province), try again in the next loop
				++i;
				continue;
			}

			int best_province_index = -1;
			int best_province_neighbor_count = 0;
			for (const auto &[province_index, count] : province_neighbor_counts) {
				if (count > best_province_neighbor_count) {
					best_province_index = province_index;
					best_province_neighbor_count = count;
				}
			}

			assert_throw(best_province_index != -1);

			//set the province to the same as the most-neighbored one
			this->tile_provinces[tile_index] = best_province_index;
			this->province_tiles[best_province_index].push_back(tile_pos);

			remaining_positions.erase(remaining_positions.begin() + i);
		}
	}
}

void map_generator::generate_countries()
{
	std::vector<const region *> potential_oceans;

	for (const region *region : region::get_all()) {
		if (!region->is_ocean()) {
			continue;
		}

		if (region->get_provinces().empty()) {
			continue;
		}

		potential_oceans.push_back(region);
	}

	vector::shuffle(potential_oceans);

	for (const region *ocean : potential_oceans) {
		if (static_cast<int>(this->generated_provinces.size()) >= this->province_count) {
			break;
		}

		this->generate_ocean(ocean);
	}

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

	const int map_area = this->get_width() * this->get_height();
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

	//assert_throw(static_cast<int>(this->generated_provinces.size()) == this->province_count);
}

bool map_generator::generate_ocean(const region *ocean)
{
	std::vector<const province *> potential_provinces;
	for (const province *province : ocean->get_provinces()) {
		potential_provinces.push_back(province);
	}

	vector::shuffle(potential_provinces);

	const std::vector<const province *> provinces = this->generate_province_group(potential_provinces, 0, nullptr);

	return !provinces.empty();
}

bool map_generator::generate_country(const country *country)
{
	if (this->generated_provinces.contains(country->get_capital_province())) {
		return false;
	}

	static constexpr int max_country_provinces = 8;

	const std::vector<const province *> provinces = this->generate_province_group(country->get_provinces(), max_country_provinces, country->get_capital_province());

	for (const province *province : provinces) {
		this->province_owners[province] = country;
	}

	return !provinces.empty();
}

std::vector<const province *> map_generator::generate_province_group(const std::vector<const province *> &potential_provinces, const int max_provinces, const province *capital_province)
{
	std::vector<const province *> provinces;

	int generated_province_count = 0;
	std::vector<int> group_province_indexes;

	for (const province *province : potential_provinces) {
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
				if (!this->can_assign_province_to_province_index(province, i)) {
					continue;
				}

				const QPoint &province_seed = this->province_seeds.at(i);

				int distance = std::numeric_limits<int>::max();

				if (!province->is_water_zone()) {
					//generate land provinces as distant from other already-generated land provinces as possible
					for (const auto &[other_province_index, other_province] : this->provinces_by_index) {
						if (other_province->is_water_zone()) {
							continue;
						}

						const QPoint &other_province_seed = this->province_seeds.at(other_province_index);
						distance = std::min(distance, point::distance_to(province_seed, other_province_seed));
					}
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

			for (const int country_province_index : group_province_indexes) {
				for (const int border_province_index : this->province_border_provinces[country_province_index]) {
					if (vector::contains(potential_province_indexes, border_province_index)) {
						continue;
					}

					if (!this->can_assign_province_to_province_index(province, border_province_index)) {
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
			if (capital_province != nullptr && province == capital_province) {
				return {};
			}

			continue;
		}

		this->provinces_by_index[province_index] = province;
		group_province_indexes.push_back(province_index);

		this->generated_provinces.insert(province);
		++generated_province_count;

		provinces.push_back(province);

		if (max_provinces > 0 && generated_province_count == max_provinces) {
			break;
		}

		if (static_cast<int>(this->generated_provinces.size()) == this->province_count) {
			break;
		}
	}

	return provinces;
}

bool map_generator::can_assign_province_to_province_index(const province *province, const int province_index) const
{
	if (this->provinces_by_index.contains(province_index)) {
		//already generated
		return false;
	}

	const QPoint &province_seed = this->province_seeds.at(province_index);

	if (this->is_tile_water(province_seed) != province->is_water_zone()) {
		//can only generate water zones on water, and land provinces on land
		return false;
	}

	if (!province->get_sites().empty()) {
		std::map<const terrain_type *, int> available_terrain_counts;

		const map *map = map::get();

		for (const QPoint &tile_pos : this->province_tiles.at(province_index)) {
			const terrain_type *terrain = map->get_tile(tile_pos)->get_terrain();
			available_terrain_counts[terrain]++;
		}

		for (const site *site : province->get_sites()) {
			if (site->get_resource() == nullptr) {
				continue;
			}

			const std::vector<const terrain_type *> &site_terrains = site->get_resource()->get_terrain_types();

			bool has_terrain = false;
			for (const terrain_type *terrain : site_terrains) {
				int &terrain_count = available_terrain_counts[terrain];
				if (terrain_count > 0) {
					has_terrain = true;
					--terrain_count;
					break;
				}
			}

			if (!has_terrain) {
				return false;
			}
		}
	}

	return true;
}

map_generator::elevation_type map_generator::get_tile_elevation_type(const QPoint &tile_pos) const
{
	const int elevation = this->tile_elevations[point::to_index(tile_pos, this->get_width())];

	if (elevation >= map_generator::min_mountain_elevation) {
		return elevation_type::mountains;
	} else if (elevation >= map_generator::min_hill_elevation) {
		return elevation_type::hills;
	} else if (elevation >= map_generator::min_land_elevation) {
		return elevation_type::flatlands;
	} else {
		return elevation_type::water;
	}
}

map_generator::forestation_type map_generator::get_tile_forestation_type(const QPoint &tile_pos) const
{
	const int forestation = this->tile_forestations[point::to_index(tile_pos, this->get_width())];

	if (forestation >= map_generator::min_forest_forestation) {
		return forestation_type::forest;
	} else {
		return forestation_type::none;
	}
}

}
