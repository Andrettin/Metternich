#include "metternich.h"

#include "map/site_map_data.h"

#include "economy/resource.h"
#include "map/map.h"
#include "map/site.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/point_util.h"

namespace metternich {

site_map_data::site_map_data(const metternich::site *site)
	: site(site)
{
	if (site->get_resource() != nullptr && site->get_resource()->is_enabled()) {
		this->set_resource(site->get_resource());
	}

	if (site->get_type() != site_type::resource || this->get_resource() != nullptr) {
		this->type = site->get_type();
	}
}

void site_map_data::set_tile_pos(const QPoint &tile_pos)
{
	assert_throw(this->get_tile_pos() == QPoint(-1, -1));

	if (tile_pos == this->get_tile_pos()) {
		return;
	}

	this->tile_pos = tile_pos;
	emit tile_pos_changed();

	this->calculate_adjacent_terrain_counts();
}

tile *site_map_data::get_tile() const
{
	if (this->get_tile_pos() != QPoint(-1, -1)) {
		return map::get()->get_tile(this->get_tile_pos());
	}

	return nullptr;
}

void site_map_data::set_type(const site_type type)
{
	if (type == this->get_type()) {
		return;
	}

	assert_throw(type != site_type::settlement);
	assert_throw(type != site_type::habitable_world);
	this->type = type;
}

void site_map_data::set_resource(const metternich::resource *resource)
{
	assert_throw(resource->is_enabled());

	this->resource = resource;
}

const province *site_map_data::get_province() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_province();
	}

	return nullptr;
}

void site_map_data::calculate_adjacent_terrain_counts()
{
	point::for_each_adjacent(this->get_tile_pos(), [this](const QPoint &adjacent_pos) {
		if (!map::get()->contains(adjacent_pos)) {
			return;
		}

		const metternich::tile *adjacent_tile = map::get()->get_tile(adjacent_pos);
		assert_throw(adjacent_tile->get_terrain() != nullptr);
		++this->adjacent_terrain_counts[adjacent_tile->get_terrain()];
	});
}

bool site_map_data::is_coastal() const
{
	if (!this->is_on_map()) {
		return false;
	}

	for (const auto &[terrain, count] : this->adjacent_terrain_counts) {
		if (terrain->is_water()) {
			return true;
		}
	}

	return false;
}

bool site_map_data::has_river() const
{
	if (!this->is_on_map()) {
		return false;
	}

	const tile *tile = this->get_tile();
	return tile->has_river();
}

bool site_map_data::is_near_water() const
{
	return this->has_river() || this->is_coastal();
}

}
