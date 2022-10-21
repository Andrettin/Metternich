#include "metternich.h"

#include "map/map.h"

#include "country/country.h"
#include "database/defines.h"
#include "economy/resource.h"
#include "game/game.h"
#include "map/direction.h"
#include "map/province.h"
#include "map/province_container.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_type.h"
#include "map/terrain_adjacency.h"
#include "map/terrain_adjacency_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace metternich {

map::map()
{
}

map::~map()
{
}

void map::create_tiles()
{
	this->tiles = std::make_unique<std::vector<tile>>();

	assert_throw(!this->get_size().isNull());

	const int tile_quantity = this->get_width() * this->get_height();
	this->tiles->reserve(tile_quantity);

	const terrain_type *base_terrain = defines::get()->get_default_base_terrain();
	const terrain_type *unexplored_terrain = defines::get()->get_unexplored_terrain();

	for (int i = 0; i < tile_quantity; ++i) {
		this->tiles->emplace_back(base_terrain, unexplored_terrain);
	}
}

void map::initialize()
{
	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);

			try {
				this->update_tile_terrain_tile(tile_pos);
			} catch (const std::exception &exception) {
				exception::report(exception);
			}
		}
	}

	province_set provinces;

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			tile *tile = this->get_tile(tile_pos);
			const province *tile_province = tile->get_province();

			if (tile_province == nullptr) {
				continue;
			}

			bool is_border_tile = false;

			point::for_each_adjacent(tile_pos, [this, tile, &tile_pos, tile_province, &is_border_tile](const QPoint &adjacent_pos) {
				if (!this->contains(adjacent_pos)) {
					is_border_tile = true;
					return;
				}

				const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
				const province *adjacent_province = adjacent_tile->get_province();

				if (tile_province != adjacent_province) {
					if (!is_border_tile) {
						province_game_data *tile_province_game_data = tile_province->get_game_data();
						if (adjacent_province != nullptr && !vector::contains(tile_province_game_data->get_border_provinces(), adjacent_province)) {
							tile_province_game_data->add_border_province(adjacent_province);
						}

						is_border_tile = true;
					}

					if (tile_province != nullptr && adjacent_province != nullptr && tile_province->is_water_zone() == adjacent_province->is_water_zone()) {
						tile->add_border_direction(offset_to_direction(adjacent_pos - tile_pos));
					}
				}
			});

			tile_province->get_game_data()->add_tile(tile_pos);

			if (is_border_tile) {
				tile->sort_border_directions();

				tile_province->get_game_data()->add_border_tile(tile_pos);
			}

			provinces.insert(tile_province);
		}
	}

	this->provinces = container::to_vector(provinces);

	emit provinces_changed();
}

void map::clear()
{
	for (country *country : country::get_all()) {
		country->reset_game_data();
	}

	for (province *province : province::get_all()) {
		province->reset_game_data();
	}

	for (site *site : site::get_all()) {
		site->reset_game_data();
	}

	this->provinces.clear();
	this->tiles.reset();
}

void map::clear_tile_game_data()
{
	if (this->tiles == nullptr) {
		return;
	}

	try {
		for (tile &tile : *this->tiles) {
			if (tile.get_development_level() != 0) {
				tile.set_development_level(0);
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to clear tile game data for the map."));
	}
}

int map::get_pos_index(const QPoint &pos) const
{
	return point::to_index(pos, this->get_width());
}

tile *map::get_tile(const QPoint &pos) const
{
	const int index = this->get_pos_index(pos);
	return &this->tiles->at(index);
}

void map::set_tile_terrain(const QPoint &tile_pos, const terrain_type *terrain)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_terrain(terrain);

	if (game::get()->is_running()) {
		//this tile and the surrounding ones need to have their displayed terrain tile updated, as adjacencies may have changed
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				const QPoint loop_tile_pos = tile_pos + QPoint(x_offset, y_offset);

				if (!this->contains(loop_tile_pos)) {
					continue;
				}

				this->update_tile_terrain_tile(loop_tile_pos);
			}
		}

		emit tile_terrain_changed(tile_pos);
	}
}

void map::update_tile_terrain_tile(const QPoint &tile_pos)
{
	static constexpr size_t direction_count = static_cast<size_t>(direction::count);
	static_assert(direction_count == terrain_adjacency::direction_count);

	try {
		tile *tile = this->get_tile(tile_pos);

		const std::vector<int> *terrain_tiles = nullptr;

		if (tile->get_terrain()->has_adjacency_tiles()) {
			terrain_adjacency adjacency;

			for (size_t i = 0; i < direction_count; ++i) {
				const direction direction = static_cast<archimedes::direction>(i);
				const QPoint offset = direction_to_offset(direction);

				const QPoint adjacent_tile_pos = tile_pos + offset;
				terrain_adjacency_type adjacency_type = terrain_adjacency_type::same;

				if (this->contains(adjacent_tile_pos)) {
					const metternich::tile *adjacent_tile = this->get_tile(adjacent_tile_pos);

					if (adjacent_tile->get_terrain() == tile->get_terrain()) {
						adjacency_type = terrain_adjacency_type::same;
					} else {
						adjacency_type = terrain_adjacency_type::other;
					}
				} else {
					adjacency_type = terrain_adjacency_type::same;
				}

				adjacency.set_direction_adjacency_type(direction, adjacency_type);
			}

			terrain_tiles = &tile->get_terrain()->get_adjacency_tiles(adjacency);
		} else {
			terrain_tiles = &tile->get_terrain()->get_tiles();
		}

		const short terrain_tile = static_cast<short>(vector::get_random(*terrain_tiles));

		tile->set_tile(terrain_tile);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to update terrain tile for tile pos " + point::to_string(tile_pos) + "."));
	}
}

void map::set_tile_province(const QPoint &tile_pos, const province *province)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_province(province);

	if (tile->get_terrain()->is_water() != province->is_water_zone()) {
		log::log_error("Tile " + point::to_string(tile_pos) + " has terrain type \"" + tile->get_terrain()->get_identifier() + "\", which has a water value that doesn't match the tile's \"" + province->get_identifier() + "\" province.");
	}
}

void map::set_tile_site(const QPoint &tile_pos, const site *site)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_site(site);

	switch (site->get_type()) {
		case site_type::settlement:
			if (tile->get_province() == nullptr || tile->get_province()->get_capital_settlement() != site) {
				log::log_error("Settlement \"" + site->get_identifier() + "\" was not placed within its province.");
			}
			break;
		case site_type::resource:
			tile->set_resource(site->get_resource());

			if (!vector::contains(tile->get_resource()->get_terrain_types(), tile->get_terrain())) {
				log::log_error("Tile " + point::to_string(tile_pos) + " has resource \"" + tile->get_resource()->get_identifier() + "\", which doesn't match its \"" + tile->get_terrain()->get_identifier() + "\" terrain type.");
			}
			break;
		default:
			break;
	}

	site->get_game_data()->set_tile_pos(tile_pos);
}

void map::set_tile_resource(const QPoint &tile_pos, const resource *resource)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_resource(resource);
}

bool map::is_tile_on_country_border(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	const country *tile_country = tile->get_owner();

	if (tile_country == nullptr) {
		return false;
	}

	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, tile_country, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			result = true;
			return true;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const country *adjacent_country = adjacent_tile->get_owner();

		if (tile_country != adjacent_country) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

QVariantList map::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

}
