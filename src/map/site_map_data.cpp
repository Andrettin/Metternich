#include "metternich.h"

#include "map/site_map_data.h"

#include "economy/resource.h"
#include "map/map.h"
#include "map/site.h"
#include "map/tile.h"
#include "util/assert_util.h"

namespace metternich {

site_map_data::site_map_data(const metternich::site *site)
	: site(site)
{
	if (site->get_resource() != nullptr && site->get_resource()->is_enabled()) {
		this->set_resource(site->get_resource());
	}
}

void site_map_data::set_tile_pos(const QPoint &tile_pos)
{
	if (tile_pos == this->get_tile_pos()) {
		return;
	}

	assert_throw(this->get_tile_pos() == QPoint(-1, -1));
	assert_throw(tile_pos.x() >= 0);
	assert_throw(tile_pos.y() >= 0);

	if (tile_pos == this->get_tile_pos()) {
		return;
	}

	this->tile_pos = tile_pos;
	emit tile_pos_changed();
}

tile *site_map_data::get_tile() const
{
	if (this->get_tile_pos() != QPoint(-1, -1)) {
		return map::get()->get_tile(this->get_tile_pos());
	}

	return nullptr;
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

bool site_map_data::is_coastal() const
{
	if (!this->is_on_map()) {
		return false;
	}

	return this->get_province()->get_map_data()->is_coastal();
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
