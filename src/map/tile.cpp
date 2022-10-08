#include "metternich.h"

#include "map/tile.h"

#include "map/terrain_type.h"
#include "util/random.h"

namespace metternich {

tile::tile(const terrain_type *base_terrain, const terrain_type *terrain) : terrain(terrain)
{
	this->base_tile_frame = random::get()->generate<short>(static_cast<short>(base_terrain->get_tiles().size()));
	this->tile_frame = random::get()->generate<short>(static_cast<short>(terrain->get_tiles().size()));
}

void tile::set_terrain(const terrain_type *terrain)
{
	if (terrain == this->get_terrain()) {
		return;
	}

	this->terrain = terrain;
	this->tile_frame = random::get()->generate<short>(static_cast<short>(terrain->get_tiles().size()));
}

}
