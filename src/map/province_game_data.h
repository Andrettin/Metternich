#pragma once

#include "economy/resource_container.h"
#include "util/qunique_ptr.h"

namespace metternich {

class country;
class culture;
class population_unit;
class province;

class province_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner READ get_owner_unconst NOTIFY owner_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY territory_changed)

public:
	explicit province_game_data(const metternich::province *province);
	province_game_data(const province_game_data &other) = delete;
	~province_game_data();

	const country *get_owner() const
	{
		return this->owner;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_owner_unconst() const
	{
		return const_cast<metternich::country *>(this->get_owner());
	}

public:
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

	void add_population_unit(qunique_ptr<population_unit> &&population_unit);
	qunique_ptr<population_unit> pop_population_unit(population_unit *population_unit);

	int get_score() const;

	province_game_data &operator =(const province_game_data &other) = delete;

signals:
	void owner_changed();
	void culture_changed();
	void territory_changed();

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
	QRect territory_rect;
	std::vector<const metternich::province *> border_provinces;
	std::vector<QPoint> tiles;
	std::vector<QPoint> border_tiles;
	std::vector<QPoint> resource_tiles;
	resource_map<int> resource_counts;
	std::vector<qunique_ptr<population_unit>> population_units;
};

}
