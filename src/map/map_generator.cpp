#include "metternich.h"

#include "map/map_generator.h"

#include "database/defines.h"
#include "map/map.h"
#include "map/province.h"
#include "util/assert_util.h"
#include "util/point_util.h"
#include "util/vector_random_util.h"

namespace metternich {

void map_generator::generate()
{
	map *map = map::get();
	map->clear();
	map->set_size(this->get_size());
	map->create_tiles();

	this->tile_provinces.resize(this->get_width() * this->get_height(), -1);

	this->generate_terrain();
	this->generate_provinces();

	map->initialize();
}

void map_generator::generate_terrain()
{
	map *map = map::get();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const terrain_type *terrain = defines::get()->get_default_province_terrain();
			map->set_tile_terrain(tile_pos, terrain);
		}
	}
}

void map_generator::generate_provinces()
{
	std::vector<const province *> potential_provinces;

	for (const province *province : province::get_all()) {
		if (province->is_water_zone()) {
			continue;
		}

		potential_provinces.push_back(province);
	}

	vector::shuffle(potential_provinces);
	assert_throw(static_cast<int>(potential_provinces.size()) >= this->province_count);

	this->province_count = std::min(this->get_width() * this->get_height() / 256, static_cast<int>(potential_provinces.size()));

	const std::vector<QPoint> seeds = this->generate_province_seeds(static_cast<size_t>(this->province_count));
	this->expand_province_seeds(seeds);

	map *map = map::get();

	for (size_t i = 0; i < this->tile_provinces.size(); ++i) {
		const int province_index = this->tile_provinces[i];
		assert_throw(province_index >= 0);

		const province *province = potential_provinces.at(province_index);
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

		if (this->tile_provinces[point::to_index(adjacent_pos, map_size)] != -1) {
			//the adjacent tile must not have a province assigned yet
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

}
