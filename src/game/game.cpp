#include "metternich.h"

#include "game/game.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/diplomacy_state.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "game/scenario.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "unit/civilian_unit.h"
#include "unit/historical_civilian_unit.h"
#include "unit/historical_civilian_unit_history.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/event_loop.h"
#include "util/path_util.h"
#include "util/size_util.h"
#include "util/thread_pool.h"

namespace metternich {

QDateTime game::normalize_date(const QDateTime &date)
{
	QDateTime normalized_date = date;
	normalized_date.setTime(QTime(0, 0, 0));

	QDate underlying_date = normalized_date.date();

	if (underlying_date.day() != 1) {
		underlying_date.setDate(underlying_date.year(), underlying_date.month(), 1);
	}

	const int month_rest = (underlying_date.month() - 1) % defines::get()->get_months_per_turn();
	if (month_rest != 0) {
		underlying_date.setDate(underlying_date.year(), underlying_date.month() - month_rest, underlying_date.day());
	}

	normalized_date.setDate(underlying_date);

	return normalized_date;
}

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

		for (const country *country : this->get_countries()) {
			for (const QPoint &border_tile_pos : country->get_game_data()->get_border_tiles()) {
				map::get()->calculate_tile_country_border_directions(border_tile_pos);
			}

			emit country->game_data_changed();
		}

		emit setup_finished();
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

		map::get()->create_minimap_image();

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
			province_game_data->clear_population_units();
		}

		//clear data related to the game (i.e. the data determined by history), but not that related only to the map
		//this is so that game setup speed can be faster if changing from one scenario to another with the same map template
		for (country *country : country::get_all()) {
			country->reset_game_data();
		}

		map::get()->clear_tile_game_data();

		this->scenario = nullptr;
		this->countries.clear();

		this->date = game::normalize_date(defines::get()->get_default_start_date());
		this->turn = 1;
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to clear the game."));
	}
}

void game::apply_history(const metternich::scenario *scenario)
{
	try {
		this->date = game::normalize_date(scenario->get_start_date());

		database::get()->load_history(scenario->get_start_date(), scenario->get_timeline());

		for (const province *province : map::get()->get_provinces()) {
			const province_history *province_history = province->get_history();
			province_game_data *province_game_data = province->get_game_data();

			province_game_data->set_owner(province_history->get_owner());
		}

		for (const country *country : this->get_countries()) {
			const country_history *country_history = country->get_history();
			country_game_data *country_game_data = country->get_game_data();

			for (const technology *technology : country_history->get_technologies()) {
				country_game_data->add_technology(technology);
			}

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

			if (site_history->get_improvement() != nullptr && site_history->get_improvement()->get_resource() != nullptr) {
				assert_throw(site->get_type() == site_type::resource);

				if (tile->get_resource() == nullptr) {
					throw std::runtime_error("Failed to set resource improvement for tile for resource site \"" + site->get_identifier() + "\", as it has no resource.");
				}

				if (tile->get_resource() != site_history->get_improvement()->get_resource()) {
					throw std::runtime_error("Failed to set resource improvement for tile for resource site \"" + site->get_identifier() + "\", as its resource is different than that of the improvement.");
				}

				tile->set_improvement(site_history->get_improvement());

				if (tile->get_improvement() != nullptr && tile->get_owner() != nullptr) {
					tile->get_owner()->get_game_data()->change_province_score(tile->get_improvement()->get_score());
				}
			}
		}

		for (const historical_civilian_unit *historical_civilian_unit : historical_civilian_unit::get_all()) {
			const historical_civilian_unit_history *historical_civilian_unit_history = historical_civilian_unit->get_history();

			if (!historical_civilian_unit_history->is_active()) {
				continue;
			}

			const site *site = historical_civilian_unit_history->get_site();

			assert_throw(site != nullptr);

			if (!site->get_game_data()->is_on_map()) {
				continue;
			}

			const QPoint tile_pos = site->get_game_data()->get_tile_pos();

			const country *owner = historical_civilian_unit->get_owner();

			if (owner == nullptr) {
				owner = map::get()->get_tile(tile_pos)->get_owner();
			}

			assert_throw(owner != nullptr);
			assert_throw(owner->get_game_data()->is_alive());

			auto civilian_unit = make_qunique<metternich::civilian_unit>(historical_civilian_unit->get_type(), owner);
			civilian_unit->set_tile_pos(tile_pos);

			owner->get_game_data()->add_civilian_unit(std::move(civilian_unit));
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to apply history."));
	}

	this->calculate_great_power_ranks();
}

void game::do_turn()
{
	for (const country *country : this->get_countries()) {
		country->get_game_data()->do_turn();
	}

	const QDateTime old_date = this->date;
	this->date = old_date.addMonths(defines::get()->get_months_per_turn());
	assert_throw(this->date != old_date);

	++this->turn;

	emit turn_changed();
}

void game::do_turn_async()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		this->do_turn();
		co_return;
	});
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

void game::calculate_great_power_ranks()
{
	//here we rank countries by province amount, but in the future this should be done by score instead
	std::vector<const metternich::country *> great_powers = game::get()->get_great_powers();

	std::sort(great_powers.begin(), great_powers.end(), [](const metternich::country *lhs, const metternich::country *rhs) {
		return lhs->get_game_data()->get_score() > rhs->get_game_data()->get_score();
	});

	for (size_t i = 0; i < great_powers.size(); ++i) {
		great_powers.at(i)->get_game_data()->set_rank(static_cast<int>(i));
	}
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

	const QSize relative_size = this->diplomatic_map_image_size / map::get()->get_size();
	this->diplomatic_map_tile_pixel_size = std::max(relative_size.width(), relative_size.height());

	std::vector<boost::asio::awaitable<void>> awaitables;

	if (map::get()->get_ocean_diplomatic_map_image().isNull()) {
		boost::asio::awaitable<void> awaitable = map::get()->create_ocean_diplomatic_map_image();
		awaitables.push_back(std::move(awaitable));
	}

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
