#pragma once

#include "util/singleton.h"

namespace metternich {

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

public:
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
	void set_tile_province(const QPoint &tile_pos, const province *province);
	void set_tile_site(const QPoint &tile_pos, const site *site);
	void set_tile_resource(const QPoint &tile_pos, const resource *resource);

	bool is_tile_on_country_border(const QPoint &tile_pos) const;
	void calculate_tile_country_border_directions(const QPoint &tile_pos);

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;

	const QImage &get_ocean_diplomatic_map_image() const
	{
		return this->ocean_diplomatic_map_image;
	}

	[[nodiscard]]
	boost::asio::awaitable<void> create_ocean_diplomatic_map_image();

	const QImage &get_minimap_image() const
	{
		return this->minimap_image;
	}

	void create_minimap_image();
	void update_minimap_rect(const QRect &tile_rect);

signals:
	void size_changed();
	void tile_terrain_changed(const QPoint &tile_pos);
	void provinces_changed();

private:
	QSize size;
	std::unique_ptr<std::vector<tile>> tiles;
	std::vector<const province *> provinces; //the provinces which are on the map
	QImage ocean_diplomatic_map_image;
	QImage minimap_image;
};

}
