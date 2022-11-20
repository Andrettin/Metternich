#include "metternich.h"

#include "map/tile.h"

#include "infrastructure/improvement.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/random.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

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

void tile::set_improvement(const metternich::improvement *improvement)
{
	this->improvement = improvement;

	if (improvement != nullptr) {
		assert_throw(this->get_resource() == improvement->get_resource());

		if (improvement->get_variation_count() > 1) {
			this->improvement_variation = random::get()->generate(improvement->get_variation_count());
		} else {
			this->improvement_variation = 0;
		}
	} else {
		this->improvement_variation = 0;
	}
}

void tile::add_river_direction(const direction direction)
{
	if (vector::contains(this->get_river_directions(), direction)) {
		return;
	}

	this->river_directions.push_back(direction);
}

int tile::get_employment_capacity() const
{
	if (this->get_improvement() != nullptr) {
		return this->get_improvement()->get_employment_capacity();
	}

	return 0;
}

}
