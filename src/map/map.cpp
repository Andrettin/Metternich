#include "metternich.h"

#include "map/map.h"

#include "map/tile.h"
#include "util/point_util.h"

namespace metternich {

map::map()
{
}

map::~map()
{
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

	emit tile_terrain_changed(tile_pos);
}

}
