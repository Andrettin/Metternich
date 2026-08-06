#include "metternich.h"

#include "map/map_grid_model.h"

#include "database/defines.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_map_data.h"
#include "map/route.h"
#include "map/route_game_data.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/point_util.h"

namespace metternich {

map_grid_model::map_grid_model()
{
	const int map_grid_width = map::get()->get_map_block_grid_width();
	const int map_grid_height = map::get()->get_map_block_grid_height();

	this->map_block_data.resize(map_grid_width * map_grid_height);

	for (int x = 0; x < map_grid_width; ++x) {
		for (int y = 0; y < map_grid_height; ++y) {
			const int map_block_index = point::to_index(x, y, map_grid_width);
			const int map_block_start_x = x * defines::get()->get_map_block_size().width();
			const int map_block_start_y = y * defines::get()->get_map_block_size().height();
			const QRect map_block_rect(QPoint(map_block_start_x, map_block_start_y), defines::get()->get_map_block_size());

			metternich::map_block_data &map_block_data = this->map_block_data.at(map_block_index);

			for (const province *province : map::get()->get_provinces()) {
				if (province->get_map_data()->get_territory_rect().intersects(map_block_rect)) {
					map_block_data.provinces.push_back(province);
				}
			}

			static constexpr int site_map_range = 16;
			const QRect map_block_site_rect(QPoint(map_block_start_x - site_map_range, map_block_start_y - site_map_range), defines::get()->get_map_block_size() + QSize(site_map_range * 2, site_map_range * 2));

			for (const site *site : map::get()->get_sites()) {
				if (map_block_site_rect.contains(site->get_map_data()->get_tile_pos())) {
					map_block_data.sites.push_back(site);
				}
			}

			for (const route *route : route::get_all()) {
				if (!route->get_game_data()->is_on_map()) {
					continue;
				}

				if (route->get_game_data()->get_map_rect().intersects(map_block_rect)) {
					map_block_data.routes.push_back(route);
				}
			}
		}
	}
}

int map_grid_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return map::get()->get_map_block_grid_height();
}

int map_grid_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return map::get()->get_map_block_grid_width();
}

QVariant map_grid_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const map_grid_model::role model_role = static_cast<map_grid_model::role>(role);

		const int map_block_x = index.column();
		const int map_block_y = index.row();
		const metternich::map_block_data &map_block_data = this->map_block_data.at(point::to_index(map_block_x, map_block_y, map::get()->get_map_block_grid_width()));

		switch (model_role) {
			case role::provinces:
				return container::to_qvariant_list(map_block_data.provinces);
			case role::sites:
				return container::to_qvariant_list(map_block_data.sites);
			case role::routes:
				return container::to_qvariant_list(map_block_data.routes);
			case role::map_block_start_x:
				return map_block_x * defines::get()->get_map_block_size().width();
			case role::map_block_start_y:
				return map_block_y * defines::get()->get_map_block_size().height();
			case role::map_block_width:
				return defines::get()->get_map_block_size().width();
			case role::map_block_height:
				return defines::get()->get_map_block_size().height();
			default:
				throw std::runtime_error(std::format("Invalid map grid model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

}
