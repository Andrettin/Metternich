#include "metternich.h"

#include "map/tile.h"

#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/vector_random_util.h"

namespace metternich {

tile::tile(const terrain_type *base_terrain, const terrain_type *terrain) : terrain(terrain)
{
	this->base_tile_frame = static_cast<short>(vector::get_random(base_terrain->get_tiles()));
}

void tile::set_terrain(const terrain_type *terrain)
{
	if (terrain == this->get_terrain()) {
		return;
	}

	this->terrain = terrain;
}

const country *tile::get_owner() const
{
	if (this->get_province() == nullptr) {
		return nullptr;
	}

	return this->get_province()->get_game_data()->get_owner();
}

const metternich::site *tile::get_settlement() const
{
	if (this->get_site() != nullptr && this->get_site()->get_type() == site_type::settlement) {
		return this->get_site();
	}

	return nullptr;
}

void tile::set_development_level(const int level)
{
	assert_throw(this->get_resource() != nullptr);

	this->development_level = level;
}

}
