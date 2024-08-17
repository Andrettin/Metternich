#pragma once

#include "economy/resource_container.h"
#include "map/terrain_type_container.h"

namespace metternich {

class province;
class site;

class province_map_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool coastal READ is_coastal CONSTANT)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY territory_changed)

public:
	explicit province_map_data(const metternich::province *province);

	void on_map_created();

	bool is_on_map() const
	{
		return !this->get_tiles().empty();
	}

	const QPoint &get_center_tile_pos() const
	{
		return this->center_tile_pos;
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	bool is_near_water() const
	{
		return this->is_coastal() || this->has_river;
	}

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	const QPoint &get_territory_rect_center() const
	{
		return this->territory_rect_center;
	}

	void calculate_territory_rect_center();

	const std::vector<const metternich::province *> &get_neighbor_provinces() const
	{
		return this->neighbor_provinces;
	}

	void add_neighbor_province(const metternich::province *province);

	const std::vector<QPoint> &get_tiles() const
	{
		return this->tiles;
	}

	void add_tile(const QPoint &tile_pos);

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	void add_border_tile(const QPoint &tile_pos);

	const std::vector<QPoint> &get_resource_tiles() const
	{
		return this->resource_tiles;
	}

	const std::vector<const site *> &get_sites() const
	{
		return this->sites;
	}

	const std::vector<const site *> &get_settlement_sites() const
	{
		return this->settlement_sites;
	}

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

	const terrain_type_map<int> &get_tile_terrain_counts() const
	{
		return this->tile_terrain_counts;
	}

signals:
	void territory_changed();

private:
	const metternich::province *province = nullptr;
	QPoint center_tile_pos = QPoint(-1, -1);
	bool coastal = false;
	bool has_river = false;
	QRect territory_rect;
	QPoint territory_rect_center = QPoint(-1, -1);
	std::vector<const metternich::province *> neighbor_provinces;
	std::vector<QPoint> tiles;
	std::vector<QPoint> border_tiles;
	std::vector<QPoint> resource_tiles;
	std::vector<const site *> sites;
	std::vector<const site *> settlement_sites; //includes all settlements, even if unbuilt
	resource_map<int> resource_counts;
	terrain_type_map<int> tile_terrain_counts;
};

}
