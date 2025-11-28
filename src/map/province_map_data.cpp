#include "metternich.h"

#include "map/province_map_data.h"

#include "database/data_entry_container.h"
#include "map/map.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"

namespace metternich {

province_map_data::province_map_data(const metternich::province *province) : province(province)
{
	if (province->get_terrain() != nullptr) {
		this->terrain = province->get_terrain();
	}
}

void province_map_data::on_map_created()
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

	if (!this->get_settlement_sites().empty()) {
		data_entry_map<holding_type, int> holding_type_max_levels;
		for (const site *holding_site : this->get_settlement_sites()) {
			if (holding_site->get_holding_type() == nullptr) {
				continue;
			}

			holding_type_max_levels[holding_site->get_holding_type()] += holding_site->get_max_holding_level();
		}

		int highest_max_holding_level = 0;
		for (const auto &[holding_type, max_holding_level] : holding_type_max_levels) {
			assert_throw(holding_type != nullptr);
			highest_max_holding_level = std::max(highest_max_holding_level, max_holding_level);
		}

		this->max_level = std::max(highest_max_holding_level, 1);
	}

	this->calculate_territory_rect_center();

	if (this->province->get_default_provincial_capital() != nullptr && this->province->get_default_provincial_capital()->get_map_data()->get_province() == this->province) {
		this->center_tile_pos = this->province->get_default_provincial_capital()->get_map_data()->get_tile_pos();
	} else {
		this->center_tile_pos = this->get_territory_rect_center();
	}

	assert_throw(this->get_center_tile_pos() != QPoint(-1, -1));

	static constexpr size_t max_holdings_per_province = 8;
	if (this->get_settlement_sites().size() > max_holdings_per_province) {
		log::log_error(std::format("Province \"{}\" has {} holding sites, more than the maximum of {}.", this->province->get_identifier(), this->get_settlement_sites().size(), max_holdings_per_province));
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

}
