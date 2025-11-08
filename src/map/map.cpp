#include "metternich.h"

#include "map/map.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "domain/country_technology.h"
#include "domain/country_turn_data.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/resource.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
#include "infrastructure/pathway.h"
#include "map/direction.h"
#include "map/province.h"
#include "map/province_container.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/route.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_adjacency.h"
#include "map/terrain_adjacency_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

#include "xbrz.h"

namespace metternich {

map::map()
{
}

map::~map()
{
}

void map::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "tile_terrains") {
		for (size_t i = 0; i < value.size(); ++i) {
			const char c = value[i];
			(*this->tiles)[i].set_terrain(terrain_type::get_by_character(c));
		}
	} else {
		throw std::runtime_error(std::format("Invalid map data property: \"{}\".", key));
	}
}

void map::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "size") {
		const QSize map_size = scope.to_size();
		this->set_size(map_size);
		this->create_tiles();
	} else if (tag == "tile_provinces") {
		int y = 0;
		scope.for_each_child([&](const gsml_data &child_scope) {
			for (size_t x = 0; x < child_scope.get_values().size(); ++x) {
				const std::string &value = child_scope.get_values()[x];
				const province *province = nullptr;
				if (value != "n") {
					const int map_province_index = std::stoi(value);
					province = this->get_provinces().at(map_province_index);
				}

				if (province != nullptr) {
					const QPoint tile_pos(static_cast<int>(x), y);
					this->set_tile_province(tile_pos, province);
				}
			}
			++y;
		});

		this->process_border_tiles();
		for (const province *province : this->get_provinces()) {
			province->get_map_data()->on_map_created();
		}
		this->initialize_diplomatic_map();
	} else {
		throw std::runtime_error(std::format("Invalid map data scope: \"{}\".", tag));
	}
}

gsml_data map::to_gsml_data() const
{
	gsml_data data("map");

	data.add_child("size", gsml_data::from_size(this->get_size()));

	std::string tile_terrains_str;
	tile_terrains_str.reserve(this->tiles->size());
	for (const tile &tile : *this->tiles) {
		tile_terrains_str += tile.get_terrain()->get_character();
	}
	data.add_property("tile_terrains", tile_terrains_str);

	std::map<const province *, size_t> map_province_indices;
	for (size_t i = 0; i < this->get_provinces().size(); ++i) {
		map_province_indices[this->get_provinces().at(i)] = i;
	}

	gsml_data tile_provinces_data("tile_provinces");
	for (int y = 0; y < this->get_height(); ++y) {
		gsml_data tile_provinces_row_data;
		for (int x = 0; x < this->get_width(); ++x) {
			const QPoint tile_pos(x, y);
			const tile *tile = this->get_tile(tile_pos);
			assert_throw(tile != nullptr);
			tile_provinces_row_data.add_value(tile->get_province() != nullptr ? std::to_string(map_province_indices.find(tile->get_province())->second) : "n");
		}
		tile_provinces_data.add_child(std::move(tile_provinces_row_data));
	}
	data.add_child(std::move(tile_provinces_data));

	return data;
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
	bool changed = true;
	while (changed) {
		changed = false;

		//assign tiles without provinces to the most-adjacent province
		for (int x = 0; x < this->get_width(); ++x) {
			for (int y = 0; y < this->get_height(); ++y) {
				const QPoint tile_pos(x, y);

				try {
					const tile *tile = this->get_tile(tile_pos);
					if (tile->get_province() != nullptr) {
						continue;
					}

					province_map<int> adjacent_province_counts;
					point::for_each_adjacent(tile_pos, [this, tile, &adjacent_province_counts](const QPoint &adjacent_pos) {
						if (!this->contains(adjacent_pos)) {
							return;
						}

						const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);

						if (tile->get_terrain()->is_water() != adjacent_tile->get_terrain()->is_water()) {
							return;
						}

						if (adjacent_tile->get_province() == nullptr) {
							return;
						}

						adjacent_province_counts[adjacent_tile->get_province()]++;
					});

					int best_count = 0;
					std::vector<const province *> best_provinces;

					for (const auto &[province, province_count] : adjacent_province_counts) {
						if (province_count > best_count) {
							best_provinces.clear();
							best_count = province_count;
						} else if (province_count < best_count) {
							continue;
						}

						best_provinces.push_back(province);
					}

					if (!best_provinces.empty()) {
						this->set_tile_province(tile_pos, vector::get_random(best_provinces));
						changed = true;
					}
				} catch (...) {
					exception::report(std::current_exception());
				}
			}
		}
	}

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);

			try {
				this->update_tile_terrain_tile(tile_pos);
			} catch (...) {
				exception::report(std::current_exception());
			}
		}
	}

	this->process_border_tiles();
	this->process_site_tiles();

	province_set provinces;
	std::vector<const site *> sites;

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			tile *tile = this->get_tile(tile_pos);
			const province *tile_province = tile->get_province();

			if (tile_province != nullptr) {
				provinces.insert(tile_province);
			}

			if (tile->get_site() != nullptr) {
				sites.push_back(tile->get_site());
			}
		}
	}

	this->provinces = container::to_vector(provinces);
	this->sites = std::move(sites);

	for (const province *province : this->get_provinces()) {
		province->get_map_data()->on_map_created();
	}

	this->initialize_diplomatic_map();

	emit provinces_changed();
	emit sites_changed();
}

void map::process_border_tiles()
{
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
						province_map_data *tile_province_map_data = tile_province->get_map_data();
						if (adjacent_province != nullptr && !vector::contains(tile_province_map_data->get_neighbor_provinces(), adjacent_province)) {
							tile_province_map_data->add_neighbor_province(adjacent_province);
						}

						is_border_tile = true;
					}

					if (tile_province != nullptr && adjacent_province != nullptr && tile_province->is_water_zone() == adjacent_province->is_water_zone()) {
						const direction border_direction = offset_to_direction(adjacent_pos - tile_pos);
						tile->add_border_direction(border_direction);

						if (tile_province->get_border_rivers().contains(adjacent_province)) {
							this->add_tile_border_river_direction(tile_pos, border_direction, adjacent_province);
						}
					}
				}
			});

			if (is_border_tile && tile->has_river()) {
				this->update_tile_terrain_tile(tile_pos);
			}

			if (is_border_tile) {
				tile->sort_border_directions();

				tile_province->get_map_data()->add_border_tile(tile_pos);
			}
		}
	}
}

void map::process_site_tiles()
{
	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			tile *tile = this->get_tile(tile_pos);
			const province *tile_province = tile->get_province();

			if (tile_province == nullptr) {
				continue;
			}

			if (tile->get_site() != nullptr) {
				tile_province->get_map_data()->process_site_tile(tile_pos);
			}
		}
	}
}

void map::clear()
{
	for (province *province : province::get_all()) {
		province->reset_map_data();
	}

	for (site *site : site::get_all()) {
		site->reset_map_data();
	}

	for (route *route : route::get_all()) {
		route->reset_game_data();
	}

	this->provinces.clear();
	this->sites.clear();
	this->tiles.reset();
	this->ocean_diplomatic_map_image = QImage();
}

void map::clear_tile_game_data()
{
	if (this->tiles == nullptr) {
		return;
	}

	try {
		for (tile &tile : *this->tiles) {
			tile.clear_improvement_variation();

			if (tile.get_civilian_unit() != nullptr) {
				tile.set_civilian_unit(nullptr);
			}

			tile.clear_country_border_directions();
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
		const QRect tile_rect(tile_pos - QPoint(1, 1), tile_pos + QPoint(1, 1));
		this->update_tile_rect_terrain_tile(tile_rect);

		emit tile_terrain_changed(tile_pos);
	}
}

void map::update_tile_terrain_tile(const QPoint &tile_pos)
{
	static constexpr size_t direction_count = static_cast<size_t>(direction::count);
	static_assert(direction_count == terrain_adjacency::direction_count);

	try {
		tile *tile = this->get_tile(tile_pos);

		if (tile->get_terrain()->get_image_filepath() == defines::get()->get_default_base_terrain()->get_image_filepath()) {
			tile->set_tile(tile->get_base_tile());
			for (size_t i = 0; i < tile->get_base_subtiles().size(); ++i) {
				tile->set_subtile(i, tile->get_base_subtiles().at(i));
			}
		} else {
			terrain_adjacency adjacency;

			if (tile->get_terrain()->has_adjacency_tiles() || tile->get_terrain()->has_adjacency_subtiles()) {
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
			}

			if (!tile->get_terrain()->get_subtiles().empty()) {
				std::array<const std::vector<int> *, 4> terrain_subtiles{};

				if (tile->get_terrain()->has_adjacency_subtiles()) {
					const std::array<terrain_adjacency, 4> subtile_adjacencies = adjacency.get_subtile_adjacencies();

					for (size_t i = 0; i < terrain_subtiles.size(); ++i) {
						terrain_subtiles[i] = &tile->get_terrain()->get_adjacency_subtiles(subtile_adjacencies.at(i));
					}
				} else {
					for (size_t i = 0; i < terrain_subtiles.size(); ++i) {
						terrain_subtiles[i] = &tile->get_terrain()->get_subtiles();
					}
				}

				for (size_t i = 0; i < terrain_subtiles.size(); ++i) {
					const short terrain_subtile = static_cast<short>(vector::get_random(*terrain_subtiles[i]));

					tile->set_subtile(i, terrain_subtile);
				}
			} else {
				const std::vector<int> *terrain_tiles = nullptr;

				if (tile->get_terrain()->has_adjacency_tiles()) {
					terrain_tiles = &tile->get_terrain()->get_adjacency_tiles(adjacency);
				} else {
					terrain_tiles = &tile->get_terrain()->get_tiles();
				}

				const short terrain_tile = static_cast<short>(vector::get_random(*terrain_tiles));

				tile->set_tile(terrain_tile);
			}
		}

		if (tile->has_river()) {
			std::array<terrain_adjacency, 4> river_subtile_adjacencies;

			for (size_t i = 0; i < direction_count; ++i) {
				const direction direction = static_cast<archimedes::direction>(i);

				for (terrain_adjacency &subtile_adjacency : river_subtile_adjacencies) {
					subtile_adjacency.set_direction_adjacency_type(direction, terrain_adjacency_type::other);
				}
			}

			if (tile->has_inner_river()) {
				terrain_adjacency river_adjacency;

				river_subtile_adjacencies[0].set_direction_adjacency_type(direction::southeast, terrain_adjacency_type::same);
				river_subtile_adjacencies[1].set_direction_adjacency_type(direction::southwest, terrain_adjacency_type::same);
				river_subtile_adjacencies[2].set_direction_adjacency_type(direction::northeast, terrain_adjacency_type::same);
				river_subtile_adjacencies[3].set_direction_adjacency_type(direction::northwest, terrain_adjacency_type::same);

				for (size_t i = 0; i < direction_count; ++i) {
					const direction direction = static_cast<archimedes::direction>(i);
					const QPoint offset = direction_to_offset(direction);

					const QPoint adjacent_tile_pos = tile_pos + offset;
					terrain_adjacency_type adjacency_type = terrain_adjacency_type::other;

					if (this->contains(adjacent_tile_pos)) {
						const metternich::tile *adjacent_tile = this->get_tile(adjacent_tile_pos);

						if (adjacent_tile->has_inner_river() || vector::contains(tile->get_river_directions(), direction)) {
							adjacency_type = terrain_adjacency_type::same;
						} else {
							adjacency_type = terrain_adjacency_type::other;
						}
					} else {
						adjacency_type = terrain_adjacency_type::same;
					}

					river_adjacency.set_direction_adjacency_type(direction, adjacency_type);
				}

				if (river_adjacency.get_direction_adjacency_type(direction::north) == terrain_adjacency_type::same) {
					river_subtile_adjacencies[0].set_direction_adjacency_type(direction::east, terrain_adjacency_type::same);
					river_subtile_adjacencies[0].set_direction_adjacency_type(direction::northeast, terrain_adjacency_type::same);
					river_subtile_adjacencies[1].set_direction_adjacency_type(direction::west, terrain_adjacency_type::same);
					river_subtile_adjacencies[1].set_direction_adjacency_type(direction::northwest, terrain_adjacency_type::same);
				}

				if (river_adjacency.get_direction_adjacency_type(direction::south) == terrain_adjacency_type::same) {
					river_subtile_adjacencies[2].set_direction_adjacency_type(direction::east, terrain_adjacency_type::same);
					river_subtile_adjacencies[2].set_direction_adjacency_type(direction::southeast, terrain_adjacency_type::same);
					river_subtile_adjacencies[3].set_direction_adjacency_type(direction::west, terrain_adjacency_type::same);
					river_subtile_adjacencies[3].set_direction_adjacency_type(direction::southwest, terrain_adjacency_type::same);
				}

				if (river_adjacency.get_direction_adjacency_type(direction::west) == terrain_adjacency_type::same) {
					river_subtile_adjacencies[0].set_direction_adjacency_type(direction::south, terrain_adjacency_type::same);
					river_subtile_adjacencies[0].set_direction_adjacency_type(direction::southwest, terrain_adjacency_type::same);
					river_subtile_adjacencies[2].set_direction_adjacency_type(direction::north, terrain_adjacency_type::same);
					river_subtile_adjacencies[2].set_direction_adjacency_type(direction::northwest, terrain_adjacency_type::same);
				}

				if (river_adjacency.get_direction_adjacency_type(direction::east) == terrain_adjacency_type::same) {
					river_subtile_adjacencies[1].set_direction_adjacency_type(direction::south, terrain_adjacency_type::same);
					river_subtile_adjacencies[1].set_direction_adjacency_type(direction::southeast, terrain_adjacency_type::same);
					river_subtile_adjacencies[3].set_direction_adjacency_type(direction::north, terrain_adjacency_type::same);
					river_subtile_adjacencies[3].set_direction_adjacency_type(direction::northeast, terrain_adjacency_type::same);
				}
			}

			for (const direction direction : tile->get_river_directions()) {
				switch (direction) {
					case direction::north:
						river_subtile_adjacencies[0].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[0].set_direction_adjacency_type(direction::northeast, terrain_adjacency_type::same);
						river_subtile_adjacencies[1].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[1].set_direction_adjacency_type(direction::northwest, terrain_adjacency_type::same);
						break;
					case direction::south:
						river_subtile_adjacencies[2].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[2].set_direction_adjacency_type(direction::southeast, terrain_adjacency_type::same);
						river_subtile_adjacencies[3].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[3].set_direction_adjacency_type(direction::southwest, terrain_adjacency_type::same);
						break;
					case direction::west:
						river_subtile_adjacencies[0].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[0].set_direction_adjacency_type(direction::southwest, terrain_adjacency_type::same);
						river_subtile_adjacencies[2].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[2].set_direction_adjacency_type(direction::northwest, terrain_adjacency_type::same);
						break;
					case direction::east:
						river_subtile_adjacencies[1].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[1].set_direction_adjacency_type(direction::southeast, terrain_adjacency_type::same);
						river_subtile_adjacencies[3].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						river_subtile_adjacencies[3].set_direction_adjacency_type(direction::northeast, terrain_adjacency_type::same);
						break;
					case direction::northwest:
						river_subtile_adjacencies[0].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						break;
					case direction::northeast:
						river_subtile_adjacencies[1].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						break;
					case direction::southwest:
						river_subtile_adjacencies[2].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						break;
					case direction::southeast:
						river_subtile_adjacencies[3].set_direction_adjacency_type(direction, terrain_adjacency_type::same);
						break;
					default:
						break;
				}
			}

			for (size_t i = 0; i < river_subtile_adjacencies.size(); ++i) {
				const std::vector<int> &river_subtiles = defines::get()->get_river_adjacency_subtiles(river_subtile_adjacencies.at(i));

				if (river_subtiles.empty()) {
					continue;
				}

				const short river_subtile = static_cast<short>(vector::get_random(river_subtiles));

				tile->set_river_subtile_frame(i, river_subtile);
			}
		}

		tile->calculate_pathway_frames();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to update terrain tile for tile pos " + point::to_string(tile_pos) + "."));
	}
}

void map::update_tile_rect_terrain_tile(const QRect &tile_rect)
{
	for (int x = tile_rect.x(); x <= tile_rect.right(); ++x) {
		for (int y = tile_rect.y(); y <= tile_rect.bottom(); ++y) {
			const QPoint tile_pos = QPoint(x, y);

			if (!this->contains(tile_pos)) {
				continue;
			}

			this->update_tile_terrain_tile(tile_pos);
		}
	}
}

void map::add_tile_border_river_direction(const QPoint &tile_pos, const direction direction, const province *border_province)
{
	tile *tile = this->get_tile(tile_pos);
	tile->add_river_direction(direction);

	switch (direction) {
		case direction::west: {
			const QPoint northwest_tile_pos = tile_pos + QPoint(-1, -1);
			if (this->contains(northwest_tile_pos)) {
				metternich::tile *northwest_tile = this->get_tile(northwest_tile_pos);
				if (northwest_tile->get_province() == border_province || northwest_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::northwest);
				}
			}

			const QPoint southwest_tile_pos = tile_pos + QPoint(-1, 1);
			if (this->contains(southwest_tile_pos)) {
				metternich::tile *southwest_tile = this->get_tile(southwest_tile_pos);
				if (southwest_tile->get_province() == border_province || southwest_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::southwest);
				}
			}
			break;
		}
		case direction::east: {
			const QPoint northeast_tile_pos = tile_pos + QPoint(1, -1);
			if (this->contains(northeast_tile_pos)) {
				metternich::tile *northeast_tile = this->get_tile(northeast_tile_pos);
				if (northeast_tile->get_province() == border_province || northeast_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::northeast);
				}
			}

			const QPoint southeast_tile_pos = tile_pos + QPoint(1, 1);
			if (this->contains(southeast_tile_pos)) {
				metternich::tile *southeast_tile = this->get_tile(southeast_tile_pos);
				if (southeast_tile->get_province() == border_province || southeast_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::southeast);
				}
			}
			break;
		}
		case direction::north: {
			const QPoint northwest_tile_pos = tile_pos + QPoint(-1, -1);
			if (this->contains(northwest_tile_pos)) {
				metternich::tile *northwest_tile = this->get_tile(northwest_tile_pos);
				if (northwest_tile->get_province() == border_province || northwest_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::northwest);
				}
			}

			const QPoint northeast_tile_pos = tile_pos + QPoint(1, -1);
			if (this->contains(northeast_tile_pos)) {
				metternich::tile *northeast_tile = this->get_tile(northeast_tile_pos);
				if (northeast_tile->get_province() == border_province || northeast_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::northeast);
				}
			}
			break;
		}
		case direction::south: {
			const QPoint southwest_tile_pos = tile_pos + QPoint(-1, 1);
			if (this->contains(southwest_tile_pos)) {
				metternich::tile *southwest_tile = this->get_tile(southwest_tile_pos);
				if (southwest_tile->get_province() == border_province || southwest_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::southwest);
				}
			}

			const QPoint southeast_tile_pos = tile_pos + QPoint(1, 1);
			if (this->contains(southeast_tile_pos)) {
				metternich::tile *southeast_tile = this->get_tile(southeast_tile_pos);
				if (southeast_tile->get_province() == border_province || southeast_tile->get_province() == tile->get_province()) {
					tile->add_river_direction(direction::southeast);
				}
			}
			break;
		}
		default:
			break;
	}
}

void map::add_tile_route_direction(const QPoint &tile_pos, const direction direction)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_direction_pathway(direction, defines::get()->get_route_pathway());
}

const metternich::province *map::get_tile_province(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	if (tile == nullptr) {
		return nullptr;
	}

	return tile->get_province();
}

void map::set_tile_province(const QPoint &tile_pos, const province *province)
{
	assert_throw(province != nullptr);

	tile *tile = this->get_tile(tile_pos);
	tile->set_province(province);
	province->get_map_data()->add_tile(tile_pos);

	if (tile->get_terrain()->is_water() != province->is_water_zone()) {
		log::log_error(std::format("Tile {} has terrain type \"{}\", which has a water value that doesn't match the tile's \"{}\" province.", point::to_string(tile_pos), tile->get_terrain()->get_identifier(), province->get_identifier()));
	}
}

void map::set_tile_site(const QPoint &tile_pos, const site *site)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_site(site);

	if (site->get_province() != nullptr && site->get_province() != tile->get_province() && site->get_province()->get_map_data()->is_on_map()) {
		log::log_error(std::format("Site \"{}\" was not placed within its province.", site->get_identifier()));
	}

	switch (site->get_type()) {
		case site_type::holding:
		case site_type::habitable_world:
			if (tile->get_province() == nullptr || (site->get_province() != nullptr && tile->get_province() != site->get_province())) {
				log::log_error(std::format("Holding \"{}\" {} was not placed within its province.", site->get_identifier(), point::to_string(tile_pos)));
			}
			break;
		case site_type::resource:
			if (site->get_map_data()->get_resource() == nullptr) {
				log::log_error(std::format("Resource site \"{}\" {} has no resource.", site->get_identifier(), point::to_string(tile_pos)));
				break;
			}
			break;
		default:
			break;
	}

	if (tile->get_resource() != nullptr) {
		if (tile->get_resource()->is_near_water() && !this->is_tile_near_water(tile_pos)) {
			log::log_error(std::format("Site \"{}\" {} has near water resource \"{}\", but is not near water.", site->get_identifier(), point::to_string(tile_pos), tile->get_resource()->get_identifier()));
		}

		if (tile->get_resource()->is_coastal() && !this->is_tile_coastal(tile_pos)) {
			log::log_error(std::format("Site \"{}\" {} has coastal resource \"{}\", but is not coastal.", site->get_identifier(), point::to_string(tile_pos), tile->get_resource()->get_identifier()));
		}

		if (!vector::contains(tile->get_resource()->get_terrain_types(), tile->get_terrain())) {
			log::log_error(std::format("Site \"{}\" {} has resource \"{}\", which doesn't match its \"{}\" terrain type.", site->get_identifier(), point::to_string(tile_pos), tile->get_resource()->get_identifier(), tile->get_terrain()->get_identifier()));
		}
	}

	site->get_map_data()->set_tile_pos(tile_pos);
}

void map::set_tile_resource_discovered(const QPoint &tile_pos, const bool discovered)
{
	tile *tile = this->get_tile(tile_pos);

	if (discovered == tile->is_resource_discovered()) {
		return;
	}

	const resource *resource = tile->get_resource();
	assert_throw(resource != nullptr);
	assert_throw(tile->get_site() != nullptr);
	tile->get_site()->get_game_data()->set_resource_discovered(discovered);

	if (discovered && resource->get_discovery_technology() != nullptr && tile->get_owner() != nullptr) {
		for (const domain *domain : game::get()->get_countries()) {
			domain_game_data *domain_game_data = domain->get_game_data();
			country_technology *country_technology = domain->get_technology();

			if (!domain_game_data->is_tile_explored(tile_pos)) {
				continue;
			}

			if (country_technology->can_gain_technology(resource->get_discovery_technology())) {
				country_technology->add_technology(resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit country_technology->technology_researched(resource->get_discovery_technology());
				}
			}
		}
	}

	emit tile_resource_changed(tile_pos);
}

void map::set_tile_direction_pathway(const QPoint &tile_pos, const direction direction, const pathway *pathway)
{
	tile *tile = this->get_tile(tile_pos);

	const metternich::pathway *old_pathway = tile->get_direction_pathway(direction);

	tile->set_direction_pathway(direction, pathway);

	if (old_pathway != nullptr) {
		tile->calculate_pathway_frame(old_pathway);
	}

	if (pathway != nullptr) {
		tile->calculate_pathway_frame(pathway);
	}

	emit tile_pathway_changed(tile_pos);
}

void map::set_tile_civilian_unit(const QPoint &tile_pos, civilian_unit *civilian_unit)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_civilian_unit(civilian_unit);

	emit tile_civilian_unit_changed(tile_pos);
}

bool map::is_tile_water(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	return tile->get_terrain() != nullptr && tile->get_terrain()->is_water();
}

bool map::is_tile_near_water(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	return tile->has_river() || this->is_tile_coastal(tile_pos);
}

bool map::is_tile_coastal(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);

	if (tile->get_terrain()->is_water()) {
		return false;
	}

	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return false;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);

		if (adjacent_tile->get_terrain()->is_water()) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

bool map::is_tile_near_celestial_body(const QPoint &tile_pos) const
{
	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return false;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);

		if (adjacent_tile->get_site() != nullptr && adjacent_tile->get_site()->is_celestial_body()) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

bool map::is_tile_on_country_border(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	const domain *tile_domain = tile->get_owner();

	if (tile_domain == nullptr) {
		return false;
	}

	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, tile_domain, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			result = true;
			return true;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const domain *adjacent_domain = adjacent_tile->get_owner();

		if (tile_domain != adjacent_domain) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

bool map::is_tile_on_province_border(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	const province *tile_province = tile->get_province();

	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, tile_province, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return false;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const province *adjacent_province = adjacent_tile->get_province();

		if (adjacent_province != tile_province) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

bool map::is_tile_on_province_border_with(const QPoint &tile_pos, const province *other_province) const
{
	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, other_province, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return false;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const province *adjacent_province = adjacent_tile->get_province();

		if (adjacent_province == other_province) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

void map::calculate_tile_country_border_directions(const QPoint &tile_pos)
{
	tile *tile = this->get_tile(tile_pos);
	const domain *tile_domain = tile->get_owner();

	tile->clear_country_border_directions();

	point::for_each_adjacent(tile_pos, [this, tile, &tile_pos, tile_domain](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const domain *adjacent_domain = adjacent_tile->get_owner();

		if (tile_domain != adjacent_domain && tile_domain != nullptr && adjacent_domain != nullptr) {
			tile->add_country_border_direction(offset_to_direction(adjacent_pos - tile_pos));
		}
	});
}

std::optional<QPoint> map::get_nearest_available_tile_pos_for_civilian_unit(const QPoint &starting_tile_pos) const
{
	const metternich::tile *starting_tile = this->get_tile(starting_tile_pos);

	std::vector<QPoint> potential_tiles = { starting_tile_pos };
	std::vector<QPoint> valid_tiles;
	point_set added_tiles = { starting_tile_pos };

	assert_throw(!this->is_tile_water(starting_tile_pos));

	while (!potential_tiles.empty() && valid_tiles.empty()) {
		for (const QPoint &tile_pos : potential_tiles) {
			const metternich::tile *tile = this->get_tile(tile_pos);
			if (tile->get_civilian_unit() == nullptr) {
				valid_tiles.push_back(tile_pos);
			}
		}

		if (valid_tiles.empty()) {
			std::vector<QPoint> next_potential_tiles;

			for (const QPoint &tile_pos : potential_tiles) {
				point::for_each_adjacent(tile_pos, [&](const QPoint &adjacent_pos) {
					if (is_tile_water(adjacent_pos)) {
						return;
					}

					const tile *adjacent_tile = this->get_tile(adjacent_pos);
					if (adjacent_tile->get_owner() != starting_tile->get_owner()) {
						return;
					}

					if (!added_tiles.contains(adjacent_pos)) {
						next_potential_tiles.push_back(adjacent_pos);
						added_tiles.insert(adjacent_pos);
					}
				});
			}

			potential_tiles = next_potential_tiles;
		}
	}

	if (valid_tiles.empty()) {
		return std::nullopt;
	}

	return vector::get_random(valid_tiles);
}

QVariantList map::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

QVariantList map::get_sites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_sites());
}

void map::initialize_diplomatic_map()
{
	const int min_tile_scale = defines::get()->get_min_diplomatic_map_tile_scale();

	const QSize min_diplomatic_map_image_size = map::min_diplomatic_map_image_size * preferences::get()->get_scale_factor();
	const QSize min_scaled_map_size = this->get_size() * min_tile_scale;

	QSize image_size = min_scaled_map_size;
	int tile_scale = min_tile_scale;
	while (image_size.width() < min_diplomatic_map_image_size.width() || image_size.height() < min_diplomatic_map_image_size.height()) {
		++tile_scale;
		image_size = this->get_size() * tile_scale;
	}

	if (image_size != this->diplomatic_map_image_size) {
		this->diplomatic_map_image_size = image_size;
		emit diplomatic_map_image_size_changed();
	}

	const QSize relative_size = this->diplomatic_map_image_size / this->get_size();
	this->diplomatic_map_tile_pixel_size = std::max(relative_size.width(), relative_size.height());
}

QCoro::Task<void> map::create_ocean_diplomatic_map_image()
{
	const int tile_pixel_size = this->get_diplomatic_map_tile_pixel_size();

	this->ocean_diplomatic_map_image = QImage(this->get_size(), QImage::Format_RGBA8888);
	this->ocean_diplomatic_map_image.fill(Qt::transparent);

	const QColor &color = defines::get()->get_ocean_color();

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos = QPoint(x, y);
			const tile *tile = this->get_tile(tile_pos);

			if (!tile->get_terrain()->is_water()) {
				continue;
			}

			this->ocean_diplomatic_map_image.setPixelColor(tile_pos, color);
		}
	}

	QImage scaled_ocean_diplomatic_map_image;

	co_await QtConcurrent::run([this, tile_pixel_size, &scaled_ocean_diplomatic_map_image]() {
		scaled_ocean_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(this->ocean_diplomatic_map_image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->ocean_diplomatic_map_image = std::move(scaled_ocean_diplomatic_map_image);

	std::vector<QPoint> border_pixels;

	const QRect image_rect = this->ocean_diplomatic_map_image.rect();

	for (int x = 0; x < this->ocean_diplomatic_map_image.width(); ++x) {
		for (int y = 0; y < this->ocean_diplomatic_map_image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = this->ocean_diplomatic_map_image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (this->ocean_diplomatic_map_image.width() - 1) || pixel_pos.y() == (this->ocean_diplomatic_map_image.height() - 1)) {
				continue;
			}

			if (pixel_color != color) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			bool is_border_pixel = false;

			point::for_each_cardinally_adjacent_until(pixel_pos, [this, &color, &image_rect, &is_border_pixel](const QPoint &adjacent_pos) {
				if (!image_rect.contains(adjacent_pos)) {
					return false;
				}

				if (this->ocean_diplomatic_map_image.pixelColor(adjacent_pos).alpha() != 0) {
					return false;
				}

				is_border_pixel = true;
				return true;
			});

			if (is_border_pixel) {
				border_pixels.push_back(pixel_pos);
			}
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : border_pixels) {
		this->ocean_diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	co_await QtConcurrent::run([this, &scale_factor, &scaled_ocean_diplomatic_map_image]() {
		scaled_ocean_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(this->ocean_diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->ocean_diplomatic_map_image = std::move(scaled_ocean_diplomatic_map_image);
}

void map::create_minimap_image()
{
	this->minimap_image = QImage(this->get_size() * defines::get()->get_minimap_tile_scale(), QImage::Format_RGBA8888);
	this->minimap_image.fill(Qt::transparent);

	this->update_minimap_rect(QRect(QPoint(0, 0), this->get_size()));
}

void map::update_minimap_rect(const QRect &tile_rect)
{
	static const centesimal_int &minimap_tile_scale = defines::get()->get_minimap_tile_scale();

	const int start_x = (tile_rect.x() * minimap_tile_scale).to_int();
	const int start_y = (tile_rect.y() * minimap_tile_scale).to_int();

	const QSize minimap_size = this->minimap_image.size();
	const int end_x = std::min(minimap_size.width() - 1, (tile_rect.right() * minimap_tile_scale).to_int());
	const int end_y = std::min(minimap_size.height() - 1, (tile_rect.bottom() * minimap_tile_scale).to_int());

	for (int x = start_x; x <= end_x; ++x) {
		for (int y = start_y; y <= end_y; ++y) {
			const QPoint pixel_pos(x, y);
			const QPoint tile_pos = pixel_pos / minimap_tile_scale;

			if (game::get()->get_player_country()->get_game_data()->is_tile_explored(tile_pos)) {
				const tile *tile = this->get_tile(tile_pos);
				const terrain_type *terrain = tile->get_terrain();

				if (terrain->is_water()) {
					this->minimap_image.setPixelColor(pixel_pos, defines::get()->get_minimap_ocean_color());
					continue;
				}

				const domain *domain = tile->get_owner();

				if (domain != nullptr) {
					this->minimap_image.setPixelColor(pixel_pos, domain->get_game_data()->get_diplomatic_map_color());
					continue;
				}
			}

			this->minimap_image.setPixelColor(pixel_pos, defines::get()->get_unexplored_terrain()->get_color());
		}
	}
}

}
