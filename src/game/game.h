#pragma once

#include "util/singleton.h"

namespace metternich {

class country;
class scenario;

class game final : public QObject, public singleton<game>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(QDateTime date READ get_date NOTIFY turn_changed)
	Q_PROPERTY(int turn READ get_turn NOTIFY turn_changed)
	Q_PROPERTY(QVariantList countries READ get_countries_qvariant_list NOTIFY countries_changed)
	Q_PROPERTY(QVariantList great_powers READ get_great_powers_qvariant_list NOTIFY countries_changed)
	Q_PROPERTY(int diplomatic_map_scale_factor READ get_diplomatic_map_scale_factor NOTIFY diplomatic_map_scale_factor_changed)
	Q_PROPERTY(metternich::country* player_country READ get_player_country_unconst WRITE set_player_country NOTIFY player_country_changed)

public:
	static constexpr QSize min_diplomatic_map_image_size = QSize(512, 256);

	static QDateTime normalize_date(const QDateTime &date);

	game();

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		emit running_changed();
	}

	Q_INVOKABLE void setup_scenario(metternich::scenario *scenario);
	Q_INVOKABLE void start();
	Q_INVOKABLE void stop();

	void clear();

	void apply_history(const metternich::scenario *scenario);

	void do_turn();
	Q_INVOKABLE void do_turn_async();

	const QDateTime &get_date() const
	{
		return this->date;
	}

	int get_turn() const
	{
		return this->turn;
	}

	const std::vector<const country *> &get_countries() const
	{
		return this->countries;
	}

	QVariantList get_countries_qvariant_list() const;
	void add_country(const country *country);
	void remove_country(const country *country);

	const std::vector<const country *> &get_great_powers() const
	{
		return this->great_powers;
	}

	QVariantList get_great_powers_qvariant_list() const;

	void calculate_great_power_ranks();

	const country *get_player_country() const
	{
		return this->player_country;
	}

private:
	//for the Qt property (pointers there can't be const)
	country *get_player_country_unconst() const
	{
		return const_cast<country *>(this->get_player_country());
	}

public:
	void set_player_country(country *country)
	{
		if (country == this->get_player_country()) {
			return;
		}

		this->player_country = country;
		emit player_country_changed();
	}

	const QImage &get_diplomatic_map_image() const
	{
		return this->diplomatic_map_image;
	}

	const QImage &get_scaled_diplomatic_map_image() const
	{
		return this->scaled_diplomatic_map_image;
	}

	void create_diplomatic_map_image();
	void update_diplomatic_map_image_rect(const QImage &rect_image, const QPoint &pos);

	int get_diplomatic_map_scale_factor() const
	{
		return this->diplomatic_map_scale_factor;
	}

	void scale_diplomatic_map();
	void set_diplomatic_map_selected_country(const country *country);

signals:
	void running_changed();
	void setup_finished();
	void turn_changed();
	void countries_changed();
	void player_country_changed();
	void diplomatic_map_scale_factor_changed();

private:
	bool running = false;
	const metternich::scenario *scenario = nullptr;
	QDateTime date; //the current date in the game
	int turn = 1;
	std::vector<const country *> countries; //the countries currently in the game, i.e. those with at least 1 province
	std::vector<const country *> great_powers;
	country *player_country = nullptr;
	QImage diplomatic_map_image;
	QImage scaled_diplomatic_map_image;
	int diplomatic_map_scale_factor = 1;
	std::vector<QPoint> scaled_diplomatic_map_border_pixels;
	const country *diplomatic_map_selected_country = nullptr;
	bool diplomatic_map_changed = false;
};

}
