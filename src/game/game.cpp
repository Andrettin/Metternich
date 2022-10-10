#include "metternich.h"

#include "game/game.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/diplomacy_state.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/scenario.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/event_loop.h"
#include "util/path_util.h"
#include "util/size_util.h"

namespace metternich {

game::game()
{
}

void game::setup_scenario(metternich::scenario *scenario)
{
	try {
		const metternich::scenario *old_scenario = this->scenario;
		this->clear();
		this->scenario = scenario;

		if (old_scenario == nullptr || old_scenario->get_map_template() != scenario->get_map_template()) {
			scenario->get_map_template()->apply();
			map::get()->initialize();
		}

		this->apply_history(scenario);
		this->create_diplomatic_map_image();
	} catch (const std::exception &exception) {
		exception::report(exception);
		std::terminate();
	}
}

void game::start()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		if (this->is_running()) {
			//already running
			co_return;
		}

		this->set_running(true);
	});
}

void game::stop()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		if (!this->is_running()) {
			//already stopped
			co_return;
		}

		this->set_running(false);
		map::get()->clear();
	});
}

void game::clear()
{
	//clear data related to the game (i.e. the data determined by history), but not that related only to the map
	//this is so that game setup speed can be faster if changing from one scenario to another with the same map template
	for (country *country : country::get_all()) {
		country->reset_game_data();
	}

	for (const province *province : province::get_all()) {
		province_game_data *province_game_data = province->get_game_data();
		province_game_data->set_owner(nullptr);
	}

	this->scenario = nullptr;
}

void game::apply_history(const metternich::scenario *scenario)
{
	database::get()->load_history(scenario->get_start_date(), scenario->get_timeline());

	for (const province *province : map::get()->get_provinces()) {
		const province_history *province_history = province->get_history();
		province_game_data *province_game_data = province->get_game_data();

		province_game_data->set_owner(province_history->get_owner());
	}

	//FIXME: iterate only through the countries which are actually on the map instead
	for (const country *country : country::get_all()) {
		const country_history *country_history = country->get_history();
		country_game_data *country_game_data = country->get_game_data();

		for (const auto &[other_country, diplomacy_state] : country_history->get_diplomacy_states()) {
			country_game_data->set_diplomacy_state(other_country, diplomacy_state);
			other_country->get_game_data()->set_diplomacy_state(country, get_diplomacy_state_counterpart(diplomacy_state));
		}
	}
}

void game::create_diplomatic_map_image()
{
	static constexpr int min_tile_scale = 2;

	const map *map = map::get();

	QSize image_size = game::min_diplomatic_map_image_size;
	if (map->get_width() >= image_size.width() || map->get_height() >= image_size.height()) {
		image_size *= min_tile_scale;
	}

	this->diplomatic_map_image = QImage(image_size, QImage::Format_RGBA8888);
	this->diplomatic_map_image.fill(Qt::black);

	this->diplomatic_map_tile_pixel_size = this->diplomatic_map_image.size() / map::get()->get_size();

	for (const country *country : country::get_all()) {
		country_game_data *country_game_data = country->get_game_data();

		if (!country_game_data->is_alive()) {
			continue;
		}

		country_game_data->create_diplomatic_map_image();
	}

	emit diplomatic_map_image_changed();
}

void game::update_diplomatic_map_image_country(const QImage &country_image, const QPoint &country_image_pos)
{
	QPainter painter(&this->diplomatic_map_image);
	painter.drawImage(country_image_pos, country_image);
	painter.end();

	emit diplomatic_map_image_changed();
}

}
