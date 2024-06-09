#include "metternich.h"

#include "map/site_map_data.h"

#include "map/site.h"

namespace metternich {

site_map_data::site_map_data(const metternich::site *site) : site(site), type(site->get_type()), resource(site->get_resource())
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

}
