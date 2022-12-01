#include "metternich.h"

#include "map/map_generator.h"

#include "country/country.h"
#include "database/defines.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/region.h"
#include "map/terrain_type.h"
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
	this->tile_elevation_types.resize(tile_count, elevation_type::none);
	this->tile_climates.resize(tile_count, climate_type::none);

	this->generate_terrain();
	this->generate_provinces();

	//assign terrain
	const terrain_type *land_terrain = defines::get()->get_default_province_terrain();
	const terrain_type *hills_terrain = terrain_type::get("barren_hills");
	const terrain_type *mountains_terrain = terrain_type::get("mountains");
	const terrain_type *water_terrain = defines::get()->get_default_water_zone_terrain();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const int tile_index = point::to_index(tile_pos, this->get_width());

			elevation_type &elevation_type = this->tile_elevation_types[tile_index];
			const bool is_water = elevation_type == elevation_type::water;

			const int province_index = this->tile_provinces[tile_index];
			const int province_seed_tile_index = point::to_index(this->province_seeds[province_index], this->get_width());
			const bool province_is_water = this->tile_elevation_types[province_seed_tile_index] == elevation_type::water;

			if (province_is_water && !is_water) {
				elevation_type = elevation_type::water;
			} else if (!province_is_water && is_water) {
				elevation_type = elevation_type::flatlands;
			}

			const terrain_type *terrain = nullptr;

			switch (elevation_type) {
				case elevation_type::water:
					terrain = water_terrain;
					break;
				case elevation_type::flatlands:
					terrain = land_terrain;
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

	map->initialize();

	for (const auto &[province, country] : this->province_owners) {
		province->get_game_data()->set_owner(country);
	}
}

void map_generator::generate_terrain()
{
	this->generate_elevation();
	//this->generate_climate();
}

void map_generator::generate_elevation()
{
	const int map_area = this->get_width() * this->get_height();
	const int mountain_seed_count = map_area / 1024;

	std::vector<QPoint> potential_positions;
	potential_positions.reserve(this->tile_elevation_types.size());

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			potential_positions.emplace_back(x, y);
		}
	}

	std::vector<QPoint> mountain_seeds;
	mountain_seeds.reserve(mountain_seed_count);

	for (int i = 0; i < mountain_seed_count; ++i) {
		while (!potential_positions.empty()) {
			QPoint random_pos = vector::take_random(potential_positions);

			const int tile_index = point::to_index(random_pos, this->get_width());
			elevation_type &tile_elevation = this->tile_elevation_types[tile_index];
			if (tile_elevation != elevation_type::none) {
				continue;
			}

			tile_elevation = elevation_type::mountains;

			mountain_seeds.push_back(std::move(random_pos));
			break;
		}
	}

	assert_throw(static_cast<int>(mountain_seeds.size()) == mountain_seed_count);

	const int min_land_tiles = map_area * map_generator::min_land_percent / 100;
	const int max_land_tiles = map_area * map_generator::max_land_percent / 100;

	int land_tile_count = 0;

	vector::merge(mountain_seeds, this->expand_elevation_seeds(mountain_seeds, elevation_type::mountains, 42, max_land_tiles - land_tile_count));
	land_tile_count += static_cast<int>(mountain_seeds.size());

	const std::vector<QPoint> hill_seeds = this->expand_elevation_seeds(mountain_seeds, elevation_type::hills, 42, max_land_tiles - land_tile_count);
	land_tile_count += static_cast<int>(hill_seeds.size());

	std::vector<QPoint> flatland_seeds = this->expand_elevation_seeds(hill_seeds, elevation_type::flatlands, 48, max_land_tiles - land_tile_count);
	land_tile_count += static_cast<int>(flatland_seeds.size());
	vector::merge(flatland_seeds, hill_seeds);

	while (land_tile_count < min_land_tiles) {
		std::vector<QPoint> new_flatland_seeds = this->expand_elevation_seeds(flatland_seeds, elevation_type::flatlands, 50, max_land_tiles - land_tile_count);
		land_tile_count += static_cast<int>(new_flatland_seeds.size());
		vector::merge(flatland_seeds, std::move(new_flatland_seeds));
	}

	//make the remaining tiles into water
	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const int tile_index = point::to_index(tile_pos, this->get_width());
			elevation_type &tile_elevation = this->tile_elevation_types[tile_index];
			if (tile_elevation != elevation_type::none) {
				continue;
			}

			tile_elevation = elevation_type::water;
		}
	}
}

std::vector<QPoint> map_generator::expand_elevation_seeds(const std::vector<QPoint> &base_seeds, const elevation_type elevation_type, const int expansion_chance, const int max_tiles)
{
	const map *map = map::get();
	const QSize &map_size = this->get_size();

	std::vector<QPoint> seeds = base_seeds;
	std::vector<QPoint> generated_seeds;

	int generated_count = 0;

	while (!seeds.empty()) {
		QPoint seed_pos = vector::take_random(seeds);

		point::for_each_cardinally_adjacent(seed_pos, [&](QPoint &&adjacent_pos) {
			if (generated_count >= max_tiles) {
				return;
			}

			if (!map->contains(adjacent_pos)) {
				return;
			}

			map_generator::elevation_type &adjacent_elevation_type = this->tile_elevation_types[point::to_index(adjacent_pos, map_size)];
			if (adjacent_elevation_type != elevation_type::none) {
				return;
			}

			//chance to expand into this tile
			if (random::get()->generate(100) < expansion_chance) {
				adjacent_elevation_type = elevation_type;
				generated_seeds.push_back(adjacent_pos);
				seeds.push_back(std::move(adjacent_pos));
				++generated_count;
			}
		});

		if (generated_count >= max_tiles) {
			break;
		}
	}

	return generated_seeds;
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

	this->province_count = map_area / 256;

	this->province_seeds = this->generate_province_seeds(static_cast<size_t>(this->province_count));
	this->expand_province_seeds(this->province_seeds);

	//ensure edge provinces are water
	const QRect map_rect(QPoint(0, 0), this->get_size());
	rect::for_each_edge_point(map_rect, [&](const QPoint &tile_pos) {
		const int tile_index = point::to_index(tile_pos, this->get_width());
		const int province_index = this->tile_provinces[tile_index];
		const QPoint &province_seed = this->province_seeds.at(province_index);
		const int province_seed_index = point::to_index(province_seed, this->get_width());
		this->tile_elevation_types[tile_index] = elevation_type::water;
		this->tile_elevation_types[province_seed_index] = elevation_type::water;
	});

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

		const auto find_iterator = this->provinces_by_index.find(province_index);
		assert_throw(find_iterator != this->provinces_by_index.end());

		if (find_iterator == this->provinces_by_index.end()) {
			continue;
		}

		const province *province = find_iterator->second;
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

			remaining_positions.erase(remaining_positions.begin() + i);
		}
	}
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
				if (this->provinces_by_index.contains(i)) {
					//already generated
					continue;
				}

				const QPoint &province_seed = this->province_seeds.at(i);
				const int province_seed_index = point::to_index(province_seed, this->get_width());

				if ((this->tile_elevation_types[province_seed_index] == elevation_type::water) != province->is_water_zone()) {
					//can only generate water zones on water, and land provinces on land
					continue;
				}

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

					if (this->provinces_by_index.contains(border_province_index)) {
						//already generated
						continue;
					}

					const QPoint &province_seed = this->province_seeds.at(border_province_index);
					const int province_seed_index = point::to_index(province_seed, this->get_width());

					if ((this->tile_elevation_types[province_seed_index] == elevation_type::water) != province->is_water_zone()) {
						//can only generate water zones on water, and land provinces on land
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

}
