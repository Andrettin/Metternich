#pragma once

#include "util/singleton.h"

namespace archimedes {
	enum class direction;
}

namespace metternich {

class civilian_unit;
class improvement;
class pathway;
class province;
class resource;
class site;
class terrain_type;
class tile;

class map final : public QObject, public singleton<map>
{
	Q_OBJECT

	Q_PROPERTY(QSize size READ get_size NOTIFY size_changed)
	Q_PROPERTY(int width READ get_width NOTIFY size_changed)
	Q_PROPERTY(int height READ get_height NOTIFY size_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QSize diplomatic_map_image_size READ get_diplomatic_map_image_size NOTIFY diplomatic_map_image_size_changed)
	Q_PROPERTY(int diplomatic_map_tile_pixel_size READ get_diplomatic_map_tile_pixel_size NOTIFY diplomatic_map_image_size_changed)

public:
	static constexpr QSize min_diplomatic_map_image_size = QSize(512, 256);

	map();
	~map();

	void create_tiles();
	void initialize();
	void clear();
	void clear_tile_game_data();

	const QSize &get_size() const
	{
		return this->size;
	}

	void set_size(const QSize &size)
	{
		if (size == this->get_size()) {
			return;
		}

		this->size = size;
		emit size_changed();
	}

	int get_width() const
	{
		return this->get_size().width();
	}

	int get_height() const
	{
		return this->get_size().height();
	}

	bool contains(const QPoint &pos) const
	{
		return (pos.x() >= 0
			&& pos.y() >= 0
			&& pos.x() < this->get_width()
			&& pos.y() < this->get_height());
	}

	int get_pos_index(const QPoint &pos) const;

	tile *get_tile(const QPoint &pos) const;
	void set_tile_terrain(const QPoint &tile_pos, const terrain_type *terrain);
	void update_tile_terrain_tile(const QPoint &tile_pos);
	void update_tile_rect_terrain_tile(const QRect &tile_rect);
	void add_tile_border_river_direction(const QPoint &tile_pos, const direction direction, const province *border_province);
	void add_tile_route_direction(const QPoint &tile_pos, const direction direction);
	void set_tile_province(const QPoint &tile_pos, const province *province);
	void set_tile_site(const QPoint &tile_pos, const site *site);
	void set_tile_resource_discovered(const QPoint &tile_pos, const bool discovered);
	void set_tile_direction_pathway(const QPoint &tile_pos, const direction direction, const pathway *pathway);
	void calculate_tile_transport_level(const QPoint &tile_pos);
	void clear_tile_transport_level(const QPoint &tile_pos);
	void set_tile_civilian_unit(const QPoint &tile_pos, civilian_unit *civilian_unit);

	bool is_tile_water(const QPoint &tile_pos) const;
	bool is_tile_near_water(const QPoint &tile_pos) const;
	bool is_tile_coastal(const QPoint &tile_pos) const;

	bool is_tile_on_country_border(const QPoint &tile_pos) const;
	bool is_tile_on_province_border_with(const QPoint &tile_pos, const province *other_province) const;
	void calculate_tile_country_border_directions(const QPoint &tile_pos);

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;

	void initialize_diplomatic_map();

	const QImage &get_ocean_diplomatic_map_image() const
	{
		return this->ocean_diplomatic_map_image;
	}

	[[nodiscard]]
	QCoro::Task<void> create_ocean_diplomatic_map_image();

	const QSize &get_diplomatic_map_image_size() const
	{
		return this->diplomatic_map_image_size;
	}

	int get_diplomatic_map_tile_pixel_size() const
	{
		return this->diplomatic_map_tile_pixel_size;
	}

	const QImage &get_minimap_image() const
	{
		return this->minimap_image;
	}

	void create_minimap_image();
	void update_minimap_rect(const QRect &tile_rect);

signals:
	void size_changed();
	void tile_terrain_changed(const QPoint &tile_pos);
	void tile_exploration_changed(const QPoint &tile_pos);
	void tile_prospection_changed(const QPoint &tile_pos);
	void tile_resource_changed(const QPoint &tile_pos);
	void tile_settlement_type_changed(const QPoint &tile_pos);
	void tile_improvement_changed(const QPoint &tile_pos);
	void tile_pathway_changed(const QPoint &tile_pos);
	void tile_transport_level_changed(const QPoint &tile_pos);
	void tile_civilian_unit_changed(const QPoint &tile_pos);
	void provinces_changed();
	void diplomatic_map_image_size_changed();

private:
	QSize size;
	std::unique_ptr<std::vector<tile>> tiles;
	std::vector<const province *> provinces; //the provinces which are on the map
	QImage ocean_diplomatic_map_image;
	QSize diplomatic_map_image_size;
	int diplomatic_map_tile_pixel_size = 1;
	QImage minimap_image;
};

}
