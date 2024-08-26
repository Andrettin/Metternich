#pragma once

#include "map/terrain_type_container.h"

Q_MOC_INCLUDE("economy/resource.h")

namespace metternich {

class resource;
class site;

class site_map_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(const metternich::resource* resource READ get_resource CONSTANT)

public:
	explicit site_map_data(const metternich::site *site);

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos);
	tile *get_tile() const;

	bool is_on_map() const
	{
		return this->get_tile_pos() != QPoint(-1, -1);
	}

	const metternich::resource *get_resource() const
	{
		return this->resource;
	}

	void set_resource(const metternich::resource *resource)
	{
		this->resource = resource;
	}

	const province *get_province() const;

	int get_adjacent_terrain_count(const terrain_type *terrain)
	{
		const auto find_iterator = this->adjacent_terrain_counts.find(terrain);
		if (find_iterator != this->adjacent_terrain_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void calculate_adjacent_terrain_counts();

	bool is_coastal() const;
	bool has_river() const;
	bool is_near_water() const;

signals:
	void tile_pos_changed();

private:
	const metternich::site *site = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	const metternich::resource *resource = nullptr;
	terrain_type_map<int> adjacent_terrain_counts;
};

}
