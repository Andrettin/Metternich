#pragma once

#include "economy/commodity_container.h"

namespace metternich {

class culture;
class improvement;
class military_unit;
class province;
class religion;
class settlement_type;
class site;
class tile;

class site_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(metternich::province* province READ get_province_unconst NOTIFY tile_pos_changed)
	Q_PROPERTY(metternich::improvement* improvement READ get_improvement_unconst NOTIFY improvement_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)

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

	const religion *get_religion() const;

	const improvement *get_improvement() const;

private:
	//for the Qt property (pointers there can't be const)
	improvement *get_improvement_unconst() const
	{
		return const_cast<improvement *>(this->get_improvement());
	}

public:
	const metternich::settlement_type *get_settlement_type() const
	{
		return this->settlement_type;
	}

	void set_settlement_type(const metternich::settlement_type *settlement_type)
	{
		if (settlement_type == this->get_settlement_type()) {
			return;
		}

		this->settlement_type = settlement_type;
		emit settlement_type_changed();
	}

	const commodity_map<int> &get_commodity_outputs() const
	{
		return this->commodity_outputs;
	}

	QVariantList get_commodity_outputs_qvariant_list() const;

	int get_commodity_output(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_outputs.find(commodity);
		if (find_iterator != this->commodity_outputs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_output(const commodity *commodity, const int output);
	void calculate_commodity_outputs();

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
	void settlement_type_changed();
	void commodity_outputs_changed();

private:
	const metternich::site *site = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	const metternich::settlement_type *settlement_type = nullptr;
	commodity_map<int> commodity_outputs;
	std::vector<military_unit *> visiting_military_units; //military units currently visiting the site
};

}
