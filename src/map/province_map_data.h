#pragma once

#include "economy/resource_container.h"
#include "map/terrain_type_container.h"

namespace metternich {

class province;
class site;
class terrain_type;

class province_map_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::terrain_type* terrain READ get_terrain CONSTANT)
	Q_PROPERTY(bool coastal READ is_coastal CONSTANT)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY territory_changed)
	Q_PROPERTY(QVariantList sites READ get_sites_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList polygon_paths READ get_polygon_paths_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList polygon_rects READ get_polygon_rects_qvariant_list CONSTANT)

public:
	explicit province_map_data(const metternich::province *province);

	void on_map_created();

	bool is_on_map() const
	{
		return !this->get_tiles().empty();
	}

	const terrain_type *get_terrain() const
	{
		return this->terrain;
	}

	void initialize_terrain();

	const QPoint &get_center_tile_pos() const
	{
		return this->center_tile_pos;
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	bool has_river() const
	{
		return this->river;
	}

	bool is_near_water() const
	{
		return this->is_coastal() || this->has_river();
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
	void calculate_center_tile_pos();

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
	void process_site_tile(const QPoint &tile_pos);
	void process_border_tile(const QPoint &tile_pos);

	const std::vector<QPoint> &get_resource_tiles() const
	{
		return this->resource_tiles;
	}

	const std::vector<const site *> &get_sites() const
	{
		return this->sites;
	}

	QVariantList get_sites_qvariant_list() const;

	const std::vector<const site *> &get_settlement_sites() const
	{
		return this->settlement_sites;
	}

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

	const std::vector<QPolygon> &get_polygons() const
	{
		return this->polygons;
	}

	QVariantList get_polygon_paths_qvariant_list() const;
	QVariantList get_polygon_rects_qvariant_list() const;

signals:
	void territory_changed();

private:
	const metternich::province *province = nullptr;
	const terrain_type *terrain = nullptr;
	QPoint center_tile_pos = QPoint(-1, -1);
	bool coastal = false;
	bool river = false;
	QRect territory_rect;
	QPoint territory_rect_center = QPoint(-1, -1);
	std::vector<const metternich::province *> neighbor_provinces;
	std::vector<QPoint> tiles;
	std::vector<QPoint> resource_tiles;
	std::vector<const site *> sites;
	std::vector<const site *> settlement_sites; //includes all settlements, even if unbuilt
	resource_map<int> resource_counts;
	std::vector<QPolygon> polygons;
};

}
