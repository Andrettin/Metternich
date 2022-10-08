#include "metternich.h"

#include "game/game.h"

#include "map/map.h"
#include "map/map_template.h"
#include "map/scenario.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/event_loop.h"
#include "util/path_util.h"

namespace metternich {

void game::setup_scenario(metternich::scenario *scenario)
{
	scenario->get_map_template()->apply();
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

void game::create_diplomatic_map_image()
{
	static constexpr QSize size(1024, 512);

	this->diplomatic_map_image = QImage(size, QImage::Format_RGBA8888);
	this->diplomatic_map_image.fill(Qt::transparent);

	const map *map = map::get();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = map->get_tile(tile_pos);
			const QColor tile_color = tile->get_terrain()->get_color();

			this->diplomatic_map_image.setPixelColor(x, y, tile_color);
		}
	}

	emit diplomatic_map_image_changed();
}

}
