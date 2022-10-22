#pragma once

#include "economy/resource_container.h"

namespace metternich {

class country;
class culture;
class province;

class province_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)

public:
	explicit province_game_data(const province *province) : province(province)
	{
	}

	const country *get_owner() const
	{
		return this->owner;
	}

	void set_owner(const country *country);

	const culture *get_culture() const;
	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	const std::vector<const metternich::province *> &get_border_provinces() const
	{
		return this->border_provinces;
	}

	void add_border_province(const metternich::province *province)
	{
		this->border_provinces.push_back(province);
	}

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

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

signals:
	void culture_changed();

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
	QRect territory_rect;
	std::vector<const metternich::province *> border_provinces;
	std::vector<QPoint> tiles;
	std::vector<QPoint> border_tiles;
	resource_map<int> resource_counts;
};

}
