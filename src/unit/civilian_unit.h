#pragma once

namespace metternich {

class civilian_unit_type;
class country;
class icon;
class tile;

class civilian_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)

public:
	explicit civilian_unit(const civilian_unit_type *type, const metternich::country *owner);

	const civilian_unit_type *get_type() const
	{
		return this->type;
	}

	void set_type(const civilian_unit_type *type)
	{
		if (type == this->get_type()) {
			return;
		}

		this->type = type;
		emit icon_changed();
	}

	const icon *get_icon() const;

private:
	//for the Qt property (pointers there can't be const)
	icon *get_icon_unconst() const
	{
		return const_cast<icon *>(this->get_icon());
	}

public:
	const country *get_owner() const
	{
		return this->owner;
	}

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos);
	tile *get_tile() const;

	void disband();

signals:
	void icon_changed();

private:
	const civilian_unit_type *type = nullptr;
	const country *owner = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
};

}
