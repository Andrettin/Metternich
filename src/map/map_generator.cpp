#include "metternich.h"

#include "map/map_generator.h"

#include "country/country.h"
#include "database/defines.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "util/assert_util.h"
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

	this->tile_provinces.resize(this->get_width() * this->get_height(), -1);

	this->generate_terrain();
	this->generate_provinces();

	map->initialize();

	for (const auto &[province, country] : this->province_owners) {
		province->get_game_data()->set_owner(country);
	}
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
	this->province_count = this->get_width() * this->get_height() / 256;

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

	static constexpr int max_powers = 7;
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
