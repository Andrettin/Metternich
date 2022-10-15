#include "metternich.h"

#include "map/map.h"

#include "country/country.h"
#include "database/defines.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_container.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"

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
	province_set provinces;

	for (int x = 0; x < this->get_width(); ++x) {
		for (int y = 0; y < this->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = this->get_tile(tile_pos);
			const province *tile_province = tile->get_province();

			if (tile_province == nullptr) {
				continue;
			}

			bool is_border_tile = false;

			point::for_each_adjacent_until(tile_pos, [this, tile_province, &is_border_tile](const QPoint &adjacent_pos) {
				if (!this->contains(adjacent_pos)) {
					is_border_tile = true;
					return true;
				}

				const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
				const province *adjacent_province = adjacent_tile->get_province();

				if (tile_province != adjacent_province) {
					province_game_data *tile_province_game_data = tile_province->get_game_data();
					if (adjacent_province != nullptr && !vector::contains(tile_province_game_data->get_border_provinces(), adjacent_province)) {
						tile_province_game_data->add_border_province(adjacent_province);
					}

					is_border_tile = true;
					return true;
				}

				return false;
			});

			tile_province->get_game_data()->add_tile(tile_pos);

			if (is_border_tile) {
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

	for (tile &tile : *this->tiles) {
		tile.set_development_level(0);
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
		emit tile_terrain_changed(tile_pos);
	}
}

void map::set_tile_province(const QPoint &tile_pos, const province *province)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_province(province);
}

void map::set_tile_site(const QPoint &tile_pos, const site *site)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_site(site);

	if (site->get_type() == site_type::resource) {
		tile->set_resource(site->get_resource());
	}

	site->get_game_data()->set_tile_pos(tile_pos);
}

void map::set_tile_resource(const QPoint &tile_pos, const commodity *resource)
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
