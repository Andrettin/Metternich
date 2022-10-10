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
		scenario->get_map_template()->apply();
		map::get()->initialize();
		this->create_diplomatic_map_image();
		this->apply_history(scenario);
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

void game::apply_history(const scenario *scenario)
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
	this->diplomatic_map_image.fill(Qt::transparent);

	this->diplomatic_map_tile_pixel_size = this->diplomatic_map_image.size() / map::get()->get_size();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = map->get_tile(tile_pos);
			const province *tile_province = tile->get_province();

			if (tile_province == nullptr) {
				this->set_diplomatic_map_image_tile_color(tile_pos, QColor(Qt::black));
			}
		}
	}

	emit diplomatic_map_image_changed();
}

void game::set_diplomatic_map_image_tile_color(const QPoint &tile_pos, const QColor &tile_color)
{
	const QSize &tile_pixel_size = this->get_diplomatic_map_tile_pixel_size();
	const QPoint top_left_pixel_pos = tile_pos * size::to_point(tile_pixel_size);

	for (int pixel_x_offset = 0; pixel_x_offset < tile_pixel_size.width(); ++pixel_x_offset) {
		for (int pixel_y_offset = 0; pixel_y_offset < tile_pixel_size.height(); ++pixel_y_offset) {
			const QPoint pixel_pos = top_left_pixel_pos + QPoint(pixel_x_offset, pixel_y_offset);
			this->diplomatic_map_image.setPixelColor(pixel_pos, tile_color);
		}
	}
}

void game::update_diplomatic_map_image_country(const QImage &country_image, const QPoint &country_image_pos)
{
	QPainter painter(&this->diplomatic_map_image);
	painter.drawImage(country_image_pos, country_image);
	painter.end();

	emit diplomatic_map_image_changed();
}

}
