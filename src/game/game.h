#pragma once

#include "util/singleton.h"

namespace metternich {

class country;
class scenario;

class game final : public QObject, public singleton<game>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)

public:
	static constexpr QSize min_diplomatic_map_image_size = QSize(1024, 512);

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

	const QImage &get_diplomatic_map_image() const
	{
		return this->diplomatic_map_image;
	}

	void create_diplomatic_map_image();

	const QSize &get_diplomatic_map_tile_pixel_size() const
	{
		return this->diplomatic_map_tile_pixel_size;
	}

	void update_diplomatic_map_image_country(const QImage &country_image, const QPoint &country_image_pos);

	const std::vector<const country *> &get_countries() const
	{
		return this->countries;
	}

	void add_country(const country *country)
	{
		this->countries.push_back(country);

		if (this->is_running()) {
			emit countries_changed();
		}
	}

	void remove_country(const country *country)
	{
		std::erase(this->countries, country);

		if (this->is_running()) {
			emit countries_changed();
		}
	}

signals:
	void running_changed();
	void diplomatic_map_image_changed();
	void countries_changed();

private:
	bool running = false;
	const metternich::scenario *scenario = nullptr;
	QImage diplomatic_map_image;
	QSize diplomatic_map_tile_pixel_size;
	std::vector<const country *> countries; //the countries currently in the game, i.e. those with at least 1 province
};

}
