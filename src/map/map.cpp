#include "metternich.h"

#include "map/map.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/resource.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
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
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/thread_pool.h"
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

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->calculate_territory_rect_center();
	}

	this->initialize_diplomatic_map();

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
	this->ocean_diplomatic_map_image = QImage();
}

void map::clear_tile_game_data()
{
	if (this->tiles == nullptr) {
		return;
	}

	try {
		for (tile &tile : *this->tiles) {
			if (tile.get_improvement() != nullptr) {
				tile.set_improvement(nullptr);
			}

			if (tile.get_civilian_unit() != nullptr) {
				tile.set_civilian_unit(nullptr);
			}

			tile.clear_employees();
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

		if (!tile->get_river_directions().empty()) {
			terrain_adjacency adjacency;

			for (size_t i = 0; i < direction_count; ++i) {
				const direction direction = static_cast<archimedes::direction>(i);
				adjacency.set_direction_adjacency_type(direction, terrain_adjacency_type::other);
			}

			for (const direction direction : tile->get_river_directions()) {
				adjacency.set_direction_adjacency_type(direction, terrain_adjacency_type::same);
			}

			const int river_frame = defines::get()->get_river_adjacency_tile(adjacency);

			if (river_frame != -1) {
				tile->set_river_frame(river_frame);
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to update terrain tile for tile pos " + point::to_string(tile_pos) + "."));
	}
}

void map::add_tile_river_direction(const QPoint &tile_pos, const direction direction, const bool riverhead)
{
	tile *tile = this->get_tile(tile_pos);
	tile->add_river_direction(direction);

	switch (direction) {
		case direction::west: {
			if (!riverhead) {
				tile->add_river_direction(direction::northwest);
				tile->add_river_direction(direction::southwest);
			}

			const QPoint west_tile_pos = tile_pos + QPoint(-1, 0);
			if (this->contains(west_tile_pos)) {
				metternich::tile *west_tile = this->get_tile(west_tile_pos);
				west_tile->add_river_direction(direction::east);

				if (!riverhead) {
					west_tile->add_river_direction(direction::northeast);
					west_tile->add_river_direction(direction::southeast);
				}
			}
			break;
		}
		case direction::east: {
			if (!riverhead) {
				tile->add_river_direction(direction::northeast);
				tile->add_river_direction(direction::southeast);
			}

			const QPoint east_tile_pos = tile_pos + QPoint(1, 0);
			if (this->contains(east_tile_pos)) {
				metternich::tile *east_tile = this->get_tile(east_tile_pos);
				east_tile->add_river_direction(direction::west);

				if (!riverhead) {
					east_tile->add_river_direction(direction::northwest);
					east_tile->add_river_direction(direction::southwest);
				}
			}
			break;
		}
		case direction::north: {
			if (!riverhead) {
				tile->add_river_direction(direction::northwest);
				tile->add_river_direction(direction::northeast);
			}

			const QPoint north_tile_pos = tile_pos + QPoint(0, -1);
			if (this->contains(north_tile_pos)) {
				metternich::tile *north_tile = this->get_tile(north_tile_pos);
				north_tile->add_river_direction(direction::south);

				if (!riverhead) {
					north_tile->add_river_direction(direction::southwest);
					north_tile->add_river_direction(direction::southeast);
				}
			}
			break;
		}
		case direction::south: {
			if (!riverhead) {
				tile->add_river_direction(direction::southwest);
				tile->add_river_direction(direction::southeast);
			}

			const QPoint south_tile_pos = tile_pos + QPoint(0, 1);
			if (this->contains(south_tile_pos)) {
				metternich::tile *south_tile = this->get_tile(south_tile_pos);
				south_tile->add_river_direction(direction::north);

				if (!riverhead) {
					south_tile->add_river_direction(direction::northwest);
					south_tile->add_river_direction(direction::northeast);
				}
			}
			break;
		}
		case direction::northwest: {
			const QPoint northwest_tile_pos = tile_pos + QPoint(-1, -1);
			if (this->contains(northwest_tile_pos)) {
				this->get_tile(northwest_tile_pos)->add_river_direction(direction::southeast);
			}
			break;
		}
		case direction::northeast: {
			const QPoint northeast_tile_pos = tile_pos + QPoint(1, -1);
			if (this->contains(northeast_tile_pos)) {
				this->get_tile(northeast_tile_pos)->add_river_direction(direction::southwest);
			}
			break;
		}
		case direction::southwest: {
			const QPoint southwest_tile_pos = tile_pos + QPoint(-1, 1);
			if (this->contains(southwest_tile_pos)) {
				this->get_tile(southwest_tile_pos)->add_river_direction(direction::northeast);
			}
			break;
		}
		case direction::southeast: {
			const QPoint southeast_tile_pos = tile_pos + QPoint(1, 1);
			if (this->contains(southeast_tile_pos)) {
				this->get_tile(southeast_tile_pos)->add_river_direction(direction::northwest);
			}
			break;
		}
		default:
			break;
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

	if (site->get_province() != nullptr && site->get_province() != tile->get_province()) {
		log::log_error("Site \"" + site->get_identifier() + "\" was not placed within its province.");
	}

	switch (site->get_type()) {
		case site_type::settlement:
			if (tile->get_province() == nullptr || tile->get_province()->get_capital_settlement() != site) {
				log::log_error("Settlement \"" + site->get_identifier() + "\" was not placed within its province.");
			}
			break;
		case site_type::resource:
			tile->set_resource(site->get_resource());

			if (tile->get_resource()->is_near_water() && !this->is_tile_near_water(tile_pos)) {
				log::log_error("Tile " + point::to_string(tile_pos) + " has near water resource \"" + tile->get_resource()->get_identifier() + "\", but is not near water.");
			}

			if (tile->get_resource()->is_coastal() && !this->is_tile_coastal(tile_pos)) {
				log::log_error("Tile " + point::to_string(tile_pos) + " has coastal resource \"" + tile->get_resource()->get_identifier() + "\", but is not coastal.");
			}

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

void map::set_tile_improvement(const QPoint &tile_pos, const improvement *improvement)
{
	tile *tile = this->get_tile(tile_pos);

	if (tile->get_improvement() != nullptr && tile->get_province() != nullptr) {
		tile->get_province()->get_game_data()->on_improvement_gained(tile->get_improvement(), -1);
	}

	tile->set_improvement(improvement);

	if (tile->get_improvement() != nullptr && tile->get_province() != nullptr) {
		tile->get_province()->get_game_data()->on_improvement_gained(tile->get_improvement(), 1);
	}

	if (game::get()->is_running()) {
		tile->get_province()->get_game_data()->reassign_workers();
	}

	emit tile_improvement_changed(tile_pos);

	if (tile->get_site() != nullptr) {
		emit tile->get_site()->get_game_data()->improvement_changed();
	}
}

void map::set_tile_civilian_unit(const QPoint &tile_pos, civilian_unit *civilian_unit)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_civilian_unit(civilian_unit);

	emit tile_civilian_unit_changed(tile_pos);
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

void map::calculate_tile_country_border_directions(const QPoint &tile_pos)
{
	tile *tile = this->get_tile(tile_pos);
	const country *tile_country = tile->get_owner();

	tile->clear_country_border_directions();

	point::for_each_adjacent(tile_pos, [this, tile, &tile_pos, tile_country](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const country *adjacent_country = adjacent_tile->get_owner();

		if (tile_country != adjacent_country && tile_country != nullptr && adjacent_country != nullptr) {
			tile->add_country_border_direction(offset_to_direction(adjacent_pos - tile_pos));
		}
	});
}

QVariantList map::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}


void map::initialize_diplomatic_map()
{
	const int min_tile_scale = defines::get()->get_min_diplomatic_map_tile_scale();

	QSize image_size = map::min_diplomatic_map_image_size * preferences::get()->get_scale_factor();
	const QSize min_scaled_map_size = this->get_size() * min_tile_scale;
	if (min_scaled_map_size.width() > image_size.width() || min_scaled_map_size.height() > image_size.height()) {
		image_size = min_scaled_map_size;
	}

	if (image_size != this->diplomatic_map_image_size) {
		this->diplomatic_map_image_size = image_size;
		emit diplomatic_map_image_size_changed();
	}

	const QSize relative_size = this->diplomatic_map_image_size / map::get()->get_size();
	this->diplomatic_map_tile_pixel_size = std::max(relative_size.width(), relative_size.height());
}

boost::asio::awaitable<void> map::create_ocean_diplomatic_map_image()
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

	co_await thread_pool::get()->co_spawn_awaitable([this, tile_pixel_size, &scaled_ocean_diplomatic_map_image]() -> boost::asio::awaitable<void> {
		scaled_ocean_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->ocean_diplomatic_map_image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
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

	co_await thread_pool::get()->co_spawn_awaitable([this, &scale_factor, &scaled_ocean_diplomatic_map_image]() -> boost::asio::awaitable<void> {
		scaled_ocean_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->ocean_diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->ocean_diplomatic_map_image = std::move(scaled_ocean_diplomatic_map_image);
}

void map::create_minimap_image()
{
	this->minimap_image = QImage(this->get_size(), QImage::Format_RGBA8888);
	this->minimap_image.fill(Qt::transparent);

	this->update_minimap_rect(QRect(QPoint(0, 0), this->get_size()));
}

void map::update_minimap_rect(const QRect &tile_rect)
{
	const int start_x = tile_rect.x();
	const int start_y = tile_rect.y();

	for (int x = start_x; x < this->get_width(); ++x) {
		for (int y = start_y; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = this->get_tile(tile_pos);
			const terrain_type *terrain = tile->get_terrain();

			if (terrain->is_water()) {
				this->minimap_image.setPixelColor(tile_pos, defines::get()->get_minimap_ocean_color());
				continue;
			}

			const country *country = tile->get_owner();

			if (country != nullptr) {
				this->minimap_image.setPixelColor(tile_pos, country->get_game_data()->get_diplomatic_map_color());
				continue;
			}

			this->minimap_image.setPixelColor(tile_pos, defines::get()->get_unexplored_terrain()->get_color());
		}
	}
}

}
