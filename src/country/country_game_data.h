#pragma once

namespace metternich {

class country;
class province;
enum class diplomacy_state;

class country_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* overlord READ get_overlord_unconst NOTIFY overlord_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QRect diplomatic_map_image_rect READ get_diplomatic_map_image_rect NOTIFY diplomatic_map_image_changed)

public:
	explicit country_game_data(const metternich::country *country) : country(country)
	{
	}

	const metternich::country *get_overlord() const
	{
		return this->overlord;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_overlord_unconst() const
	{
		return const_cast<metternich::country *>(this->get_overlord());
	}

public:
	void set_overlord(const metternich::country *overlord);

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;
	void add_province(const province *province);
	void remove_province(const province *province);

	bool is_alive() const
	{
		return !this->get_provinces().empty();
	}

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	void calculate_territory_rect();

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	diplomacy_state get_diplomacy_state(const metternich::country *other_country) const;
	void set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state);

	const QColor &get_diplomatic_map_color() const;

	const QImage &get_diplomatic_map_image() const
	{
		return this->diplomatic_map_image;
	}

	void create_diplomatic_map_image();

	const QRect &get_diplomatic_map_image_rect() const
	{
		return this->diplomatic_map_image_rect;
	}

	const QImage &get_selected_diplomatic_map_image() const
	{
		return this->selected_diplomatic_map_image;
	}

signals:
	void overlord_changed();
	void provinces_changed();
	void diplomatic_map_image_changed();

private:
	const metternich::country *country = nullptr;
	const metternich::country *overlord = nullptr;
	std::vector<const province *> provinces;
	QRect territory_rect;
	std::vector<QPoint> border_tiles;
	std::map<const metternich::country *, diplomacy_state> diplomacy_states;
	QImage diplomatic_map_image;
	QImage selected_diplomatic_map_image;
	std::vector<QPoint> diplomatic_map_border_pixels;
	QRect diplomatic_map_image_rect;
};

}
