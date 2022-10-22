#include "metternich.h"

#include "game/game.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/diplomacy_state.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/scenario.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/event_loop.h"
#include "util/path_util.h"
#include "util/size_util.h"
#include "util/thread_pool.h"

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

		emit countries_changed();
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
		this->clear();
		map::get()->clear();
	});
}

void game::clear()
{
	try {
		for (const province *province : province::get_all()) {
			province_game_data *province_game_data = province->get_game_data();
			province_game_data->set_owner(nullptr);
		}

		//clear data related to the game (i.e. the data determined by history), but not that related only to the map
		//this is so that game setup speed can be faster if changing from one scenario to another with the same map template
		for (country *country : country::get_all()) {
			country->reset_game_data();
		}

		map::get()->clear_tile_game_data();

		this->scenario = nullptr;
		this->countries.clear();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to clear the game."));
	}
}

void game::apply_history(const metternich::scenario *scenario)
{
	try {
		database::get()->load_history(scenario->get_start_date(), scenario->get_timeline());

		for (const province *province : map::get()->get_provinces()) {
			const province_history *province_history = province->get_history();
			province_game_data *province_game_data = province->get_game_data();

			province_game_data->set_owner(province_history->get_owner());
		}

		for (const country *country : this->get_countries()) {
			const country_history *country_history = country->get_history();
			country_game_data *country_game_data = country->get_game_data();

			for (const auto &[other_country, diplomacy_state] : country_history->get_diplomacy_states()) {
				country_game_data->set_diplomacy_state(other_country, diplomacy_state);
				other_country->get_game_data()->set_diplomacy_state(country, get_diplomacy_state_counterpart(diplomacy_state));
			}
		}

		for (const site *site : site::get_all()) {
			const site_game_data *site_game_data = site->get_game_data();
			tile *tile = site_game_data->get_tile();

			if (tile == nullptr) {
				continue;
			}

			const site_history *site_history = site->get_history();

			if (site_history->get_development_level() != 0) {
				assert_throw(site->get_type() == site_type::resource);

				if (tile->get_resource() == nullptr) {
					throw std::runtime_error("Failed to set development level for tile for resource site \"" + site->get_identifier() + "\", as it has no resource.");
				}

				tile->set_development_level(site_history->get_development_level());
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to apply history."));
	}

	for (const country *country : this->get_countries()) {
		emit country->game_data_changed();
	}
}

QVariantList game::get_countries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_countries());
}

void game::add_country(const country *country)
{
	this->countries.push_back(country);

	if (country->is_great_power()) {
		this->great_powers.push_back(country);
	}

	if (this->is_running()) {
		emit countries_changed();
	}
}

void game::remove_country(const country *country)
{
	std::erase(this->countries, country);

	if (country->is_great_power()) {
		std::erase(this->great_powers, country);
	}

	if (this->is_running()) {
		emit countries_changed();
	}
}

QVariantList game::get_great_powers_qvariant_list() const
{
	return container::to_qvariant_list(this->get_great_powers());
}

void game::create_diplomatic_map_image()
{
	const int min_tile_scale = defines::get()->get_min_diplomatic_map_tile_scale();

	const map *map = map::get();

	QSize image_size = game::min_diplomatic_map_image_size;
	const QSize min_scaled_map_size = map->get_size() * min_tile_scale;
	if (min_scaled_map_size.width() > image_size.width() || min_scaled_map_size.height() > image_size.height()) {
		image_size = min_scaled_map_size;
	}

	if (image_size != this->diplomatic_map_image_size) {
		this->diplomatic_map_image_size = image_size;
		emit diplomatic_map_image_size_changed();
	}

	this->diplomatic_map_tile_pixel_size = this->diplomatic_map_image_size / map::get()->get_size();

	std::vector<boost::asio::awaitable<void>> awaitables;

	for (const country *country : this->get_countries()) {
		country_game_data *country_game_data = country->get_game_data();

		boost::asio::awaitable<void> awaitable = country_game_data->create_diplomatic_map_image();
		awaitables.push_back(std::move(awaitable));
	}

	thread_pool::get()->co_spawn_sync([&awaitables]() -> boost::asio::awaitable<void> {
		for (boost::asio::awaitable<void> &awaitable : awaitables) {
			co_await std::move(awaitable);
		}
	});
}

}
