#include "metternich.h"

#include "game/game.h"

#include "map/map.h"
#include "map/map_template.h"
#include "map/scenario.h"
#include "util/assert_util.h"
#include "util/event_loop.h"
#include "util/path_util.h"

namespace metternich {

void game::start_scenario(metternich::scenario *scenario)
{
	event_loop::get()->co_spawn([this, scenario]() -> boost::asio::awaitable<void> {
		if (this->is_running()) {
			//already running
			co_return;
		}

		const map_template *map_template = scenario->get_map_template();
		map_template->apply();

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

}
