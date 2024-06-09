#pragma once

Q_MOC_INCLUDE("economy/resource.h")

namespace metternich {

class resource;
class site;
enum class site_type;

class site_map_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(const metternich::resource* resource READ get_resource CONSTANT)

public:
	explicit site_map_data(const metternich::site *site);

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos);

	site_type get_type() const
	{
		return this->type;
	}

	void set_type(const site_type type)
	{
		this->type = type;
	}

	const metternich::resource *get_resource() const
	{
		return this->resource;
	}

	void set_resource(const metternich::resource *resource)
	{
		this->resource = resource;
	}

signals:
	void tile_pos_changed();

private:
	const metternich::site *site = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	site_type type;
	const metternich::resource *resource = nullptr;
};

}
