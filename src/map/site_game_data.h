#pragma once

namespace metternich {

class culture;
class province;
class site;
class tile;

class site_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)

public:
	explicit site_game_data(const site *site) : site(site)
	{
	}

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

	const province *get_province() const;
	const culture *get_culture() const;

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

signals:
	void tile_pos_changed();
	void culture_changed();

private:
	const metternich::site *site = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
};

}
