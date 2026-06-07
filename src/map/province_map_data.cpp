#include "metternich.h"

#include "map/province_map_data.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "map/map.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"
#include "util/vector_util.h"

namespace metternich {

province_map_data::province_map_data(const metternich::province *province) : province(province)
{
	if (province->get_terrain() != nullptr) {
		this->terrain = province->get_terrain();
	}
}

void province_map_data::on_map_created()
{
	this->initialize_terrain();

	/*
	if (this->get_tiles().size() <= 10000) {
		std::vector<QPoint> tiles_to_process = this->get_border_tiles();
		for (size_t i = 0; i < tiles_to_process.size(); ++i) {
			const QPoint &tile_pos = tiles_to_process.at(i);

			QPolygon polygon;
			polygon.append(tile_pos);

			QPoint current_tile_pos = tile_pos;
			bool changed = true;
			while (changed) {
				changed = false;

				for (int multiplier = 1; multiplier >= -1; multiplier -= 2) {
					bool added_east = false;
					for (int x_offset = 1;; ++x_offset) {
						const QPoint &east_tile_pos = current_tile_pos + QPoint(x_offset * multiplier, 0);
						if (multiplier > 0) {
							if (east_tile_pos.x() >= map::get()->get_width()) {
								break;
							}
						} else {
							if (east_tile_pos.x() < 0) {
								break;
							}
						}

						if (vector::contains(tiles_to_process, east_tile_pos)) {
							if (added_east)
								polygon.back() = east_tile_pos;
							else {
								polygon.append(east_tile_pos);
								added_east = true;
							}
							std::erase(tiles_to_process, east_tile_pos);
							changed = true;
						} else {
							break;
						}
					}
					current_tile_pos = polygon.back();

					bool added_south = false;
					for (int y_offset = 1;; ++y_offset) {
						const QPoint &south_tile_pos = current_tile_pos + QPoint(0, y_offset * multiplier);
						if (multiplier > 0) {
							if (south_tile_pos.y() >= map::get()->get_height()) {
								break;
							}
						} else {
							if (south_tile_pos.y() < 0) {
								break;
							}
						}

						if (vector::contains(tiles_to_process, south_tile_pos)) {
							if (added_south)
								polygon.back() = south_tile_pos;
							else {
								polygon.append(south_tile_pos);
								added_south = true;
							}
							std::erase(tiles_to_process, south_tile_pos);
							changed = true;
						} else {
							break;
						}
					}
					current_tile_pos = polygon.back();
				}

				current_tile_pos = polygon.back();
			}

			polygon.append(polygon.front());
			this->polygons.push_back(std::move(polygon));
		}

		bool changed = true;
		while (changed) {
			changed = false;

			for (size_t i = 0; i < this->polygons.size(); ++i) {
				QPolygon &first_polygon = this->polygons.at(i);

				for (size_t j = i + 1; j < this->polygons.size();) {
					const QPolygon &second_polygon = this->polygons.at(j);

					if (first_polygon.intersects(second_polygon)) {
						first_polygon = first_polygon.united(second_polygon);
						this->polygons.erase(this->polygons.begin() + j);
						changed = true;
					} else {
						++j;
					}
				}
			}
		}
	}
	*/

	this->calculate_territory_rect_center();

	if (this->province->get_default_provincial_capital() != nullptr && this->province->get_default_provincial_capital()->get_map_data()->get_province() == this->province && this->province->get_default_provincial_capital()->get_map_data()->is_on_map()) {
		this->center_tile_pos = this->province->get_default_provincial_capital()->get_map_data()->get_tile_pos();
	} else {
		this->center_tile_pos = this->get_territory_rect_center();
	}

	assert_throw(this->get_center_tile_pos() != QPoint(-1, -1));

	if (this->get_settlement_sites().size() > province::max_holdings) {
		log::log_error(std::format("Province \"{}\" has {} holding sites, more than the maximum of {}.", this->province->get_identifier(), this->get_settlement_sites().size(), province::max_holdings));
	}
}

void province_map_data::initialize_terrain()
{
	if (this->get_terrain() == nullptr) {
		const terrain_type *best_terrain = nullptr;
		int best_terrain_count = 0;
		for (const auto &[tile_terrain, count] : this->get_tile_terrain_counts()) {
			if (count > best_terrain_count) {
				best_terrain = tile_terrain;
				best_terrain_count = count;
			}
		}

		this->terrain = best_terrain;
		assert_throw(this->get_terrain() != nullptr);
	}

	//set the terrain type for the province's sites too
	for (const site *site : this->get_sites()) {
		if (site->get_map_data()->get_terrain() == nullptr) {
			site->get_map_data()->set_terrain(this->get_terrain());
		}
	}
}

void province_map_data::calculate_territory_rect_center()
{
	const int tile_count = static_cast<int>(this->get_tiles().size());

	assert_throw(tile_count > 0);

	QPoint sum(0, 0);

	for (const QPoint &tile_pos : this->get_tiles()) {
		sum += tile_pos;
	}

	this->territory_rect_center = QPoint(sum.x() / tile_count, sum.y() / tile_count);
}

void province_map_data::add_neighbor_province(const metternich::province *province)
{
	this->neighbor_provinces.push_back(province);

	if (province->is_sea() || province->is_bay()) {
		this->coastal = true;
	}
}

void province_map_data::add_tile(const QPoint &tile_pos)
{
	this->tiles.push_back(tile_pos);

	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_terrain() != nullptr) {
		++this->tile_terrain_counts[tile->get_terrain()];
	}

	if (tile->has_river()) {
		this->has_river = true;
	}
}

void province_map_data::process_site_tile(const QPoint &tile_pos)
{
	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_resource() != nullptr) {
		this->resource_tiles.push_back(tile_pos);
		++this->resource_counts[tile->get_resource()];
	}

	if (tile->get_site() != nullptr) {
		assert_throw(tile->get_site()->get_map_data()->is_on_map());

		this->sites.push_back(tile->get_site());

		if (tile->get_site()->is_settlement()) {
			this->settlement_sites.push_back(tile->get_site());
		}
	}
}

void province_map_data::add_border_tile(const QPoint &tile_pos)
{
	this->border_tiles.push_back(tile_pos);

	if (this->get_territory_rect().isNull()) {
		this->territory_rect = QRect(tile_pos, QSize(1, 1));
	} else {
		if (tile_pos.x() < this->territory_rect.x()) {
			this->territory_rect.setX(tile_pos.x());
		} else if (tile_pos.x() > this->territory_rect.right()) {
			this->territory_rect.setRight(tile_pos.x());
		}
		if (tile_pos.y() < this->territory_rect.y()) {
			this->territory_rect.setY(tile_pos.y());
		} else if (tile_pos.y() > this->territory_rect.bottom()) {
			this->territory_rect.setBottom(tile_pos.y());
		}
	}

	emit territory_changed();
}

QVariantList province_map_data::get_sites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_sites());
}

QVariantList province_map_data::get_polygon_paths_qvariant_list() const
{
	QVariantList polygon_paths;

	for (const QPolygon &polygon : this->get_polygons()) {
		std::string polygon_path;

		bool first = true;

		for (const QPoint &point : polygon) {
			if (first) {
				polygon_path += std::format("M {} {}", (point.x() * defines::get()->get_min_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int(), (point.y() * defines::get()->get_min_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int());
				first = false;
			} else {
				polygon_path += std::format(" L {} {}", (point.x() * defines::get()->get_min_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int(), (point.y() * defines::get()->get_min_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int());
			}
		}

		polygon_path += "z";

		polygon_paths.append(QString::fromStdString(polygon_path));
	}

	return polygon_paths;
}

QVariantList province_map_data::get_polygon_rects_qvariant_list() const
{
	std::vector<QRect> polygon_rects;

	for (const QPolygon &polygon : this->get_polygons()) {
		polygon_rects.push_back(polygon.boundingRect());
	}

	return container::to_qvariant_list(polygon_rects);
}

}
