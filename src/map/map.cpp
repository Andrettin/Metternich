#include "metternich.h"

#include "map/map.h"

#include "database/defines.h"
#include "game/game.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/point_util.h"

namespace metternich {

map::map()
{
}

map::~map()
{
}

void map::initialize()
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

void map::clear()
{
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

}
