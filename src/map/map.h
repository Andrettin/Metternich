#pragma once

#include "util/singleton.h"

namespace metternich {

class terrain_type;

class map final : public QObject, public singleton<map>
{
	Q_OBJECT

public:
	const QSize &get_size() const
	{
		return this->size;
	}

	void set_size(const QSize &size)
	{
		this->size = size;
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

signals:
	void tile_terrain_changed(const QPoint &tile_pos);

private:
	QSize size;
};

}
