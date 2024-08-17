#include "metternich.h"

#include "map/map_generator.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/resource.h"
#include "map/elevation_type.h"
#include "map/forestation_type.h"
#include "map/map.h"
#include "map/moisture_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/temperature_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"
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
	this->tile_moistures.resize(tile_count, -1);
	this->tile_forestations.resize(tile_count, -1);

	this->generate_provinces();
	this->generate_terrain();
	this->generate_countries();

	//assign provinces
	for (size_t i = 0; i < this->tile_provinces.size(); ++i) {
		const int province_index = this->tile_provinces[i];
		assert_throw(province_index >= 0);

		const auto find_iterator = this->provinces_by_index.find(province_index);

		if (find_iterator == this->provinces_by_index.end()) {
			continue;
		}

		const province *province = find_iterator->second;
		assert_throw(province != nullptr);
		map->set_tile_province(point::from_index(static_cast<int>(i), this->get_width()), province);
	}

	this->generate_sites();

	map->initialize();

	for (const auto &[province, country] : this->province_owners) {
		province_game_data *province_game_data = province->get_game_data();
		province_game_data->set_owner(country);

		//add population
		for (const site *settlement : province_game_data->get_settlement_sites()) {
			const int population_unit_count = settlement->get_game_data()->is_capital() ? 4 : 1;
			const population_class *population_class = country->get_game_data()->get_default_population_class();

			const culture *culture = country->get_culture();
			const religion *religion = country->get_default_religion();
			const population_type *population_type = culture->get_population_class_type(population_class);

			for (int i = 0; i < population_unit_count; ++i) {
				settlement->get_game_data()->create_population_unit(population_type, culture, religion, culture->get_default_phenotype());
			}
		}
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
		this->sea_zones.insert(province_index);
	});

	this->generate_elevation();
	this->generate_moisture();
	this->generate_forestation();

	//assign terrain
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

			const elevation_type elevation_type = this->get_tile_elevation_type(tile_pos);
			const temperature_type temperature_type = this->get_tile_temperature_type(tile_pos);
			const moisture_type moisture_type = this->get_tile_moisture_type(tile_pos);
			const forestation_type forestation_type = this->get_tile_forestation_type(tile_pos);

			const terrain_type *terrain = terrain_type::get_by_biome(elevation_type, temperature_type, moisture_type, forestation_type);

			map->set_tile_terrain(tile_pos, terrain);
		}
	}

	//build the province tiles by terrain map
	for (size_t i = 0; i < this->province_tiles.size(); ++i) {
		this->province_tiles_by_terrain.emplace_back();
		this->province_near_water_tiles_by_terrain.emplace_back();

		for (const QPoint &tile_pos : this->province_tiles.at(i)) {
			const terrain_type *terrain = map->get_tile(tile_pos)->get_terrain();
			this->province_tiles_by_terrain[i][terrain].push_back(tile_pos);

			if (map->is_tile_near_water(tile_pos)) {
				this->province_near_water_tiles_by_terrain[i][terrain].push_back(tile_pos);
			}
		}
	}

	//designate which water zones are seas and which are lakes
	//lakes are water zones which have no connection to the seas on the edge of the map

	std::vector<int> sea_zones = container::to_vector(this->sea_zones);

	for (size_t i = 0; i < sea_zones.size(); ++i) {
		const int province_index = sea_zones.at(i);

		for (const int border_province_index : this->province_border_provinces[province_index]) {
			const QPoint &province_seed = this->province_seeds.at(border_province_index);

			if (!this->is_tile_water(province_seed)) {
				continue;
			}

			if (this->sea_zones.contains(border_province_index)) {
				continue;
			}

			sea_zones.push_back(border_province_index);
			this->sea_zones.insert(border_province_index);
		}
	}

	for (int i = 0; i < this->province_count; ++i) {
		const QPoint &province_seed = this->province_seeds.at(i);

		if (!this->is_tile_water(province_seed)) {
			continue;
		}

		if (this->sea_zones.contains(i)) {
			continue;
		}

		this->lakes.insert(i);
	}
}

void map_generator::generate_elevation()
{
	const std::vector<QPoint> elevation_seeds = this->generate_tile_value_seeds(this->tile_elevations, 2048);
	this->expand_tile_value_seeds(elevation_seeds, this->tile_elevations, 50);
}

void map_generator::generate_moisture()
{
	const std::vector<QPoint> seeds = this->generate_tile_value_seeds(this->tile_moistures, 256);
	this->expand_tile_value_seeds(seeds, this->tile_moistures, 100);
}

void map_generator::generate_forestation()
{
	const std::vector<QPoint> seeds = this->generate_tile_value_seeds(this->tile_forestations, 256);
	this->expand_tile_value_seeds(seeds, this->tile_forestations, 50);
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

void map_generator::expand_tile_value_seeds(const std::vector<QPoint> &base_seeds, std::vector<int> &tile_values, const int max_decrease)
{
	const map *map = map::get();

	std::vector<QPoint> seeds = vector::shuffled(base_seeds);

	while (!seeds.empty()) {
		std::vector<QPoint> new_seeds;

		for (const QPoint &seed_pos : seeds) {
			const int tile_value = tile_values[point::to_index(seed_pos, this->get_width())];

			point::for_each_cardinally_adjacent(seed_pos, [&](QPoint &&adjacent_pos) {
				if (!map->contains(adjacent_pos)) {
					return;
				}

				int &adjacent_tile_value = tile_values[point::to_index(adjacent_pos, this->get_width())];
				if (adjacent_tile_value != -1) {
					return;
				}

				adjacent_tile_value = std::max(0, tile_value - random::get()->generate_in_range(0, max_decrease));
				new_seeds.push_back(std::move(adjacent_pos));
			});
		}

		seeds = std::move(new_seeds);
		vector::shuffle(seeds);
	}
}

void map_generator::generate_provinces()
{
	const int map_area = this->get_width() * this->get_height();

	this->province_count = map_area / 64;

	this->province_seeds = this->generate_province_seeds(static_cast<size_t>(this->province_count));
	this->expand_province_seeds(this->province_seeds);
}

std::vector<QPoint> map_generator::generate_province_seeds(const size_t seed_count)
{
	static constexpr int min_province_seed_distance = 6;

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

	if (province_seeds.size() != seed_count) {
		throw std::runtime_error("Could only generate " + std::to_string(province_seeds.size()) + " province seeds out of the required " + std::to_string(seed_count) + ".");
	}

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

	std::vector<QPoint> seeds = vector::shuffled(base_seeds);

	while (!seeds.empty()) {
		std::vector<QPoint> new_seeds;

		for (const QPoint &seed_pos : seeds) {
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
				new_seeds.push_back(std::move(seed_pos));
			}

			QPoint adjacent_pos = vector::get_random(adjacent_positions);
			this->tile_provinces[point::to_index(adjacent_pos, map_size)] = province_index;
			this->province_tiles[province_index].push_back(adjacent_pos);

			new_seeds.push_back(std::move(adjacent_pos));
		}

		seeds = std::move(new_seeds);
		vector::shuffle(seeds);
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

	std::vector<const province *> potential_seas;
	std::vector<const province *> potential_lakes;

	for (const province *province : province::get_all()) {
		if (province->is_sea() || province->is_bay()) {
			potential_seas.push_back(province);
		} else if (province->is_lake()) {
			potential_lakes.push_back(province);
		}
	}

	vector::shuffle(potential_seas);
	vector::shuffle(potential_lakes);

	for (const province *province : potential_seas) {
		if (static_cast<int>(this->generated_provinces.size()) == this->province_count) {
			break;
		}

		std::vector<int> group_province_indexes;
		this->generate_province(province, group_province_indexes);
	}

	for (const province *province : potential_lakes) {
		if (static_cast<int>(this->generated_provinces.size()) == this->province_count) {
			break;
		}

		std::vector<int> group_province_indexes;
		this->generate_province(province, group_province_indexes);
	}

	std::vector<const country *> potential_powers;
	std::vector<const country *> potential_minor_nations;

	for (const country *country : country::get_all()) {
		if (country->get_core_provinces().empty()) {
			continue;
		}

		if (!vector::contains(country->get_eras(), this->era)) {
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

	if (static_cast<int>(this->generated_provinces.size()) != this->province_count) {
		log::log_error(std::to_string(this->generated_provinces.size()) + " provinces were generated, but " + std::to_string(this->province_count) + " were needed.");
	}
}

bool map_generator::generate_ocean(const region *ocean)
{
	std::vector<const province *> potential_provinces;
	for (const province *province : ocean->get_provinces()) {
		potential_provinces.push_back(province);
	}

	const std::vector<const province *> provinces = this->generate_province_group(potential_provinces, 0, nullptr);

	return !provinces.empty();
}

bool map_generator::generate_country(const country *country)
{
	if (this->generated_provinces.contains(country->get_core_provinces().at(0))) {
		return false;
	}

	static constexpr int max_country_provinces = 8;

	std::vector<const province *> potential_provinces;
	for (const province *province : country->get_core_provinces()) {
		potential_provinces.push_back(province);
	}

	const std::vector<const province *> provinces = this->generate_province_group(potential_provinces, max_country_provinces, country->get_core_provinces().at(0));

	for (const province *province : provinces) {
		this->province_owners[province] = country;
	}

	return !provinces.empty();
}

std::vector<const province *> map_generator::generate_province_group(const std::vector<const province *> &potential_provinces, const int max_provinces, const province *capital_province)
{
	std::vector<const province *> provinces;
	std::vector<int> group_province_indexes;

	if (capital_province != nullptr) {
		const int province_index = this->generate_province(capital_province, group_province_indexes);
		if (province_index == -1) {
			return {};
		}

		provinces.push_back(capital_province);
	}

	for (const province *province : vector::shuffled(potential_provinces)) {
		const int province_index = this->generate_province(province, group_province_indexes);
		if (province_index == -1) {
			continue;
		}

		provinces.push_back(province);

		if (max_provinces > 0 && static_cast<int>(group_province_indexes.size()) == max_provinces) {
			break;
		}

		if (static_cast<int>(this->generated_provinces.size()) == this->province_count) {
			break;
		}
	}

	return provinces;
}

int map_generator::generate_province(const province *province, std::vector<int> &group_province_indexes)
{
	if (this->generated_provinces.contains(province)) {
		return -1;
	}

	int province_index = -1;
	const size_t group_generated_province_count = group_province_indexes.size();

	if (group_generated_province_count == 0) {
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
		std::map<int, int> province_index_border_counts;
		std::set<int> checked_province_indexes;

		for (const int country_province_index : group_province_indexes) {
			for (const int border_province_index : this->province_border_provinces[country_province_index]) {
				if (checked_province_indexes.contains(border_province_index)) {
					if (!province_index_border_counts.contains(border_province_index)) {
						//already checked for suitability and deemed unsuitable
						continue;
					}
				} else {
					checked_province_indexes.insert(border_province_index);
					//make the suitability check if it hasn't been done yet
					if (!this->can_assign_province_to_province_index(province, border_province_index)) {
						continue;
					}
				}

				++province_index_border_counts[border_province_index];
			}
		}

		std::vector<int> best_province_indexes;
		int best_border_count = 0;
		for (const auto &[border_province_index, border_count] : province_index_border_counts) {
			if (border_count > best_border_count) {
				best_province_indexes.clear();
				best_border_count = border_count;
			}

			if (border_count == best_border_count) {
				best_province_indexes.push_back(border_province_index);
			}
		}

		if (!best_province_indexes.empty()) {
			province_index = vector::get_random(best_province_indexes);
		}
	}

	if (province_index == -1) {
		return province_index;
	}

	this->provinces_by_index[province_index] = province;
	group_province_indexes.push_back(province_index);

	this->generated_provinces.insert(province);

	return province_index;
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

	if ((province->is_sea() || province->is_bay()) && !this->sea_zones.contains(province_index)) {
		return false;
	}

	if (province->is_lake() && !this->lakes.contains(province_index)) {
		return false;
	}

	if (province->is_bay()) {
		//bays can only appear adjacent to land provinces
		bool has_adjacent_land = false;
		for (const int border_province_index : this->province_border_provinces.find(province_index)->second) {
			const QPoint &border_province_seed = this->province_seeds.at(border_province_index);

			if (!this->is_tile_water(border_province_seed)) {
				has_adjacent_land = true;
				break;
			}
		}

		if (!has_adjacent_land) {
			return false;
		}
	}

	if (!province->get_sites().empty()) {
		terrain_type_map<int> available_terrain_counts;
		for (const auto &[terrain, tiles] : this->province_tiles_by_terrain[province_index]) {
			available_terrain_counts[terrain] = static_cast<int>(tiles.size());
		}

		terrain_type_map<int> available_near_water_terrain_counts;
		for (const auto &[terrain, tiles] : this->province_near_water_tiles_by_terrain[province_index]) {
			available_near_water_terrain_counts[terrain] = static_cast<int>(tiles.size());
		}

		for (const site *site : province->get_sites()) {
			const resource *resource = site->get_map_data()->get_resource();
			if (resource == nullptr) {
				continue;
			}

			const std::vector<const terrain_type *> &site_terrains = resource->get_terrain_types();

			bool has_terrain = false;
			for (const terrain_type *terrain : site_terrains) {
				int &terrain_count = resource->is_near_water() ? available_near_water_terrain_counts[terrain] : available_terrain_counts[terrain];
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

void map_generator::generate_sites()
{
	map *map = map::get();

	for (const auto &[province_index, province] : this->provinces_by_index) {
		assert_throw(province != nullptr);

		const QPoint &province_seed = this->province_seeds[province_index];

		//place capital settlement
		if (province->get_provincial_capital() != nullptr) {
			map->set_tile_site(province_seed, province->get_provincial_capital());

			//change non-flatlands or forested terrain to unforested flatlands for settlements
			const terrain_type *tile_terrain = map->get_tile(province_seed)->get_terrain();
			if (tile_terrain->get_elevation_type() != elevation_type::flatlands || tile_terrain->get_forestation_type() != forestation_type::none) {
				const temperature_type temperature_type = this->get_tile_temperature_type(province_seed);
				const moisture_type moisture_type = this->get_tile_moisture_type(province_seed);

				const terrain_type *terrain = terrain_type::get_by_biome(elevation_type::flatlands, temperature_type, moisture_type, forestation_type::none);

				map->set_tile_terrain(province_seed, terrain);
			}
		}

		for (const site *site : province->get_sites()) {
			if (site->get_type() != site_type::resource && !site->is_settlement()) {
				continue;
			}

			const resource *resource = site->get_map_data()->get_resource();
			const std::vector<const terrain_type *> &site_terrains = resource->get_terrain_types();

			const terrain_type_map<std::vector<QPoint>> &province_tiles_by_terrain = resource->is_near_water() ? this->province_near_water_tiles_by_terrain[province_index] : this->province_tiles_by_terrain[province_index];

			std::vector<QPoint> potential_positions;

			for (const terrain_type *terrain : site_terrains) {
				const auto find_iterator = province_tiles_by_terrain.find(terrain);
				if (find_iterator == province_tiles_by_terrain.end()) {
					continue;
				}

				for (const QPoint &tile_pos : find_iterator->second) {
					if (map->get_tile(tile_pos)->get_site() != nullptr) {
						continue;
					}

					potential_positions.push_back(tile_pos);
				}
			}

			assert_throw(!potential_positions.empty());

			const QPoint &site_pos = vector::get_random(potential_positions);
			map->set_tile_site(site_pos, site);
		}
	}
}

elevation_type map_generator::get_tile_elevation_type(const QPoint &tile_pos) const
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

bool map_generator::is_tile_water(const QPoint &tile_pos) const
{
	return this->get_tile_elevation_type(tile_pos) == elevation_type::water;
}

int map_generator::get_tile_temperature(const QPoint &tile_pos) const
{
	//temperature is a function of latitude and elevation
	const int tile_index = point::to_index(tile_pos, this->get_width());
	const int colatitude = this->get_tile_colatitude(tile_pos);
	const int land_elevation = std::max(0, this->tile_elevations[tile_index] - map_generator::min_land_elevation);
	return std::max(0, colatitude - land_elevation / 2);
}

temperature_type map_generator::get_tile_temperature_type(const QPoint &tile_pos) const
{
	const int temperature = this->get_tile_temperature(tile_pos);

	if (temperature >= map_generator::min_tropical_temperature) {
		return temperature_type::tropical;
	} else if (temperature >= map_generator::min_temperate_temperature) {
		return temperature_type::temperate;
	} else {
		return temperature_type::cold;
	}
}

moisture_type map_generator::get_tile_moisture_type(const QPoint &tile_pos) const
{
	const int moisture = this->tile_moistures[point::to_index(tile_pos, this->get_width())];

	if (moisture >= map_generator::min_wet_moisture) {
		return moisture_type::wet;
	} else if (moisture >= map_generator::min_moist_moisture) {
		return moisture_type::moist;
	} else if (moisture >= map_generator::min_dry_moisture) {
		return moisture_type::dry;
	} else if (moisture >= map_generator::min_semi_arid_moisture) {
		return moisture_type::semi_arid;
	} else {
		return moisture_type::arid;
	}
}

forestation_type map_generator::get_tile_forestation_type(const QPoint &tile_pos) const
{
	const int forestation = this->tile_forestations[point::to_index(tile_pos, this->get_width())];

	if (forestation >= map_generator::min_forest_forestation) {
		return forestation_type::forest;
	} else {
		return forestation_type::none;
	}
}

}
