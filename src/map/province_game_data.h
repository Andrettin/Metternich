#pragma once

namespace metternich {

class country;
class province;

class province_game_data final
{
public:
	explicit province_game_data(const province *province) : province(province)
	{
	}

	const country *get_owner() const
	{
		return this->owner;
	}

	void set_owner(const country *country);

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

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	void add_border_tile(const QPoint &tile_pos);

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
	QRect territory_rect;
	std::vector<const metternich::province *> border_provinces;
	std::vector<QPoint> border_tiles;
};

}
