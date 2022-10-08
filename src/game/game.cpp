#include "metternich.h"

#include "game/game.h"

#include "country/country.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/scenario.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/event_loop.h"
#include "util/path_util.h"

namespace metternich {

game::game()
{
	static constexpr QSize diplomatic_map_image_size(1024, 512);

	this->diplomatic_map_image = QImage(diplomatic_map_image_size, QImage::Format_RGBA8888);
	this->diplomatic_map_image.fill(Qt::transparent);
}

void game::setup_scenario(metternich::scenario *scenario)
{
	scenario->get_map_template()->apply();
	map::get()->initialize();
	this->apply_history(scenario);
	this->create_diplomatic_map_image();
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
}

void game::create_diplomatic_map_image()
{
	this->diplomatic_map_image.fill(Qt::transparent);

	const map *map = map::get();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = map->get_tile(tile_pos);
			const province *tile_province = tile->get_province();

			if (tile_province == nullptr) {
				this->diplomatic_map_image.setPixelColor(x, y, QColor(Qt::black));
				continue;
			}

			const province_game_data *province_game_data = tile_province->get_game_data();
			const country *owner = province_game_data->get_owner();

			if (owner == nullptr) {
				continue;
			}

			const QColor tile_color = owner->get_color();

			this->diplomatic_map_image.setPixelColor(x, y, tile_color);
		}
	}

	emit diplomatic_map_image_changed();
}

}
