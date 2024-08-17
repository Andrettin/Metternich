#include "metternich.h"

#include "map/site_map_data.h"

#include "map/map.h"
#include "map/site.h"
#include "map/tile.h"

namespace metternich {

site_map_data::site_map_data(const metternich::site *site) : site(site), resource(site->get_resource())
{
}

void site_map_data::set_tile_pos(const QPoint &tile_pos)
{
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

const province *site_map_data::get_province() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_province();
	}

	return nullptr;
}

}
