#pragma once

namespace metternich {

class culture;
class improvement;
class military_unit;
class province;
class site;
class tile;

class site_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(metternich::province* province READ get_province_unconst NOTIFY tile_pos_changed)
	Q_PROPERTY(metternich::improvement* improvement READ get_improvement_unconst NOTIFY improvement_changed)
	Q_PROPERTY(int employee_count READ get_employee_count NOTIFY improvement_changed)
	Q_PROPERTY(int employment_capacity READ get_employment_capacity NOTIFY improvement_changed)
	Q_PROPERTY(int production_modifier READ get_production_modifier NOTIFY improvement_changed)

public:
	explicit site_game_data(const site *site) : site(site)
	{
	}

	void reset_non_map_data();

	void do_turn();

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos)
	{
		if (tile_pos == this->get_tile_pos()) {
			return;
		}

		this->tile_pos = tile_pos;
		emit tile_pos_changed();
	}

	tile *get_tile() const;

	bool is_on_map() const
	{
		return this->tile_pos != QPoint(-1, -1);
	}

	const province *get_province() const;

private:
	//for the Qt property (pointers there can't be const)
	province *get_province_unconst() const
	{
		return const_cast<province *>(this->get_province());
	}

public:
	const culture *get_culture() const;

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

	const improvement *get_improvement() const;

private:
	//for the Qt property (pointers there can't be const)
	improvement *get_improvement_unconst() const
	{
		return const_cast<improvement *>(this->get_improvement());
	}

public:
	int get_employee_count() const;
	int get_employment_capacity() const;
	int get_production_modifier() const;

	const std::vector<military_unit *> &get_visiting_military_units() const
	{
		return this->visiting_military_units;
	}

	void add_visiting_military_unit(military_unit *military_unit)
	{
		this->visiting_military_units.push_back(military_unit);
	}

	void remove_visiting_military_unit(const military_unit *military_unit)
	{
		std::erase(this->visiting_military_units, military_unit);
	}

signals:
	void tile_pos_changed();
	void culture_changed();
	void improvement_changed();

private:
	const metternich::site *site = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	std::vector<military_unit *> visiting_military_units; //military units currently visiting the site
};

}
