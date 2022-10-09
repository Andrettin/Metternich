#include "metternich.h"

#include "map/map.h"

#include "country/country.h"
#include "database/defines.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_container.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/point_util.h"

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

	for (const tile &tile : *this->tiles) {
		if (tile.get_province() == nullptr) {
			continue;
		}

		provinces.insert(tile.get_province());
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

void map::set_tile_settlement(const QPoint &tile_pos, const site *settlement)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_settlement(settlement);

	settlement->get_game_data()->set_tile_pos(tile_pos);
}

QVariantList map::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

}
