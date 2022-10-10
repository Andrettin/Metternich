#pragma once

namespace metternich {

class country;
class province;
enum class diplomacy_state;

class country_game_data final
{
public:
	explicit country_game_data(const metternich::country *country) : country(country)
	{
	}

	const metternich::country *get_overlord() const
	{
		return this->overlord;
	}

	void add_province(const province *province);
	void remove_province(const province *province);

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	diplomacy_state get_diplomacy_state(const metternich::country *other_country) const;
	void set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state);

	const QColor &get_diplomatic_map_color() const;

private:
	const metternich::country *country = nullptr;
	const metternich::country *overlord = nullptr;
	std::vector<const province *> provinces;
	std::vector<QPoint> border_tiles;
	std::map<const metternich::country *, diplomacy_state> diplomacy_states;
};

}
