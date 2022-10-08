#pragma once

#include "util/singleton.h"

namespace metternich {

class province;
class terrain_type;
class tile;

class map final : public QObject, public singleton<map>
{
	Q_OBJECT

	Q_PROPERTY(QSize size READ get_size NOTIFY size_changed)
	Q_PROPERTY(int width READ get_width NOTIFY size_changed)
	Q_PROPERTY(int height READ get_height NOTIFY size_changed)

public:
	map();
	~map();

	void create_tiles();
	void initialize();
	void clear();

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
	void set_tile_province(const QPoint &tile_pos, const province *province);

signals:
	void size_changed();
	void tile_terrain_changed(const QPoint &tile_pos);

private:
	QSize size;
	std::unique_ptr<std::vector<tile>> tiles;
	std::vector<const province *> provinces; //the provinces which are on the map
};

}
