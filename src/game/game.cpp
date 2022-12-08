#include "metternich.h"

#include "game/game.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/culture.h"
#include "country/diplomacy_state.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "game/scenario.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/map_generator.h"
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
#include "time/era.h"
#include "unit/civilian_unit.h"
#include "unit/historical_civilian_unit.h"
#include "unit/historical_civilian_unit_history.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/event_loop.h"
#include "util/log_util.h"
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

void game::create_random_map(const QSize &map_size, metternich::era *era)
{
	try {
		this->clear();

		map_generator map_generator(map_size, era);
		map_generator.generate();

		this->date = game::normalize_date(era->get_start_date());

		this->on_setup_finished();
	} catch (const std::exception &exception) {
		exception::report(exception);
		std::terminate();
	}
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
		this->on_setup_finished();
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

		for (const province *province : map::get()->get_provinces()) {
			province_game_data *province_game_data = province->get_game_data();
			province_game_data->assign_workers();
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
			province_game_data->reset_non_map_data();
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

			const country *owner = province_history->get_owner();

			province_game_data->set_owner(owner);
		}

		for (const country *country : this->get_countries()) {
			const country_history *country_history = country->get_history();
			country_game_data *country_game_data = country->get_game_data();

			for (const technology *technology : country_history->get_technologies()) {
				country_game_data->add_technology(technology);
			}

			for (const auto &[other_country, diplomacy_state] : country_history->get_diplomacy_states()) {
				if (!other_country->get_game_data()->is_alive()) {
					continue;
				}

				country_game_data->set_diplomacy_state(other_country, diplomacy_state);
				other_country->get_game_data()->set_diplomacy_state(country, get_diplomacy_state_counterpart(diplomacy_state));
			}

			for (const auto &[other_country, consulate] : country_history->get_consulates()) {
				if (!other_country->get_game_data()->is_alive()) {
					continue;
				}

				country_game_data->set_consulate(other_country, consulate);
				other_country->get_game_data()->set_consulate(country, consulate);
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

				if (tile->get_improvement() != nullptr && tile->get_province() != nullptr) {
					tile->get_province()->get_game_data()->on_improvement_gained(tile->get_improvement(), 1);
				}
			}

			const province *tile_province = tile->get_province();

			if (tile_province != nullptr) {
				province_game_data *tile_province_game_data = tile_province->get_game_data();

				for (const auto &[building_slot_type, building] : site_history->get_buildings()) {
					const building_type *slot_building = tile_province_game_data->get_slot_building(building_slot_type);
					if (slot_building == nullptr || slot_building->get_score() < building->get_score()) {
						tile_province_game_data->set_slot_building(building_slot_type, building);
					}
				}
			}
		}

		this->apply_population_history();

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
			const tile *tile = map::get()->get_tile(tile_pos);

			const country *owner = historical_civilian_unit->get_owner();

			if (owner == nullptr) {
				owner = tile->get_owner();
			}

			assert_throw(owner != nullptr);
			assert_throw(owner->get_game_data()->is_alive());

			const province *home_province = historical_civilian_unit->get_home_province();
			if (home_province == nullptr) {
				if (tile->get_province()->get_game_data()->get_owner() == owner) {
					home_province = tile->get_province();
				} else if (!owner->get_game_data()->is_under_anarchy()) {
					home_province = owner->get_capital_province();
				} else {
					continue;
				}
			}
			assert_throw(home_province != nullptr);

			const culture *culture = historical_civilian_unit->get_culture();
			if (culture == nullptr) {
				if (home_province->get_game_data()->get_culture() != nullptr) {
					culture = home_province->get_game_data()->get_culture();
				} else {
					culture = owner->get_culture();
				}
			}
			assert_throw(culture != nullptr);

			const population_type *population_type = historical_civilian_unit->get_population_type();
			if (population_type == nullptr) {
				population_type = culture->get_population_class_type(defines::get()->get_default_population_class());
			}
			assert_throw(population_type != nullptr);

			const phenotype *phenotype = historical_civilian_unit->get_phenotype();
			if (phenotype == nullptr) {
				phenotype = culture->get_default_phenotype();
			}
			assert_throw(phenotype != nullptr);

			auto civilian_unit = make_qunique<metternich::civilian_unit>(historical_civilian_unit->get_type(), owner, home_province, population_type, culture, phenotype);
			civilian_unit->set_tile_pos(tile_pos);

			owner->get_game_data()->add_civilian_unit(std::move(civilian_unit));
		}

		for (const province *province : map::get()->get_provinces()) {
			province_game_data *province_game_data = province->get_game_data();
			province_game_data->calculate_culture();
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to apply history."));
	}
}

void game::apply_population_history()
{
	country_map<population_group_map<int>> country_populations;

	for (const province *province : map::get()->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		province_history *province_history = province->get_history();
		province_game_data *province_game_data = province->get_game_data();

		province_history->initialize_population();

		for (const auto &[group_key, population] : province_history->get_population_groups()) {
			if (population <= 0) {
				continue;
			}

			const int64_t remaining_population = this->apply_historical_population_group_to_province(group_key, population, province);

			if (remaining_population != 0 && province_game_data->get_owner() != nullptr) {
				//add the remaining population to remaining population data for the owner remaining population
				country_populations[province_game_data->get_owner()][group_key] += remaining_population;
			}
		}
	}

	for (auto &[country, population_groups] : country_populations) {
		const province *capital_province = country->get_capital_province();
		province_game_data *capital_province_game_data = capital_province->get_game_data();

		if (capital_province_game_data->get_owner() != country) {
			continue;
		}

		//initialize entries for groups with less defined properties, since we might need to use them
		population_groups[population_group_key()];
		const population_group_map<int> population_groups_copy = population_groups;
		for (const auto &[group_key, population] : population_groups_copy) {
			if (group_key.type != nullptr) {
				population_groups[population_group_key(group_key.type, group_key.culture, nullptr)];
				population_groups[population_group_key(group_key.type, nullptr, group_key.phenotype)];
				population_groups[population_group_key(group_key.type, nullptr, nullptr)];
			}

			if (group_key.culture != nullptr) {
				population_groups[population_group_key(group_key.type, group_key.culture, nullptr)];
				population_groups[population_group_key(nullptr, group_key.culture, group_key.phenotype)];
				population_groups[population_group_key(nullptr, group_key.culture, nullptr)];
			}

			if (group_key.phenotype != nullptr) {
				population_groups[population_group_key(group_key.type, nullptr, group_key.phenotype)];
				population_groups[population_group_key(nullptr, group_key.culture, group_key.phenotype)];
				population_groups[population_group_key(nullptr, nullptr, group_key.phenotype)];
			}
		}

		for (const auto &[group_key, population] : population_groups) {
			if (population <= 0) {
				continue;
			}

			const int64_t remaining_population = this->apply_historical_population_group_to_province(group_key, population, capital_province);

			//add the remaining population to broader groups
			if (remaining_population > 0 && !group_key.is_empty()) {
				population_group_key group_key_copy = group_key;

				if (group_key.phenotype != nullptr) {
					group_key_copy.phenotype = nullptr;
				} else if (group_key.culture != nullptr) {
					group_key_copy.culture = nullptr;
				} else if (group_key.type != nullptr) {
					group_key_copy.type = nullptr;
				} else {
					assert_throw(false);
				}

				const auto group_find_iterator = population_groups.find(group_key_copy);
				assert_throw(group_find_iterator != population_groups.end());
				group_find_iterator->second += remaining_population;
			}

			if (remaining_population != 0 && group_key.is_empty()) {
				//if this is general population data, then add the remaining population to the stored population growth
				country->get_game_data()->change_population_growth(remaining_population * defines::get()->get_population_growth_threshold() / defines::get()->get_population_per_unit());
			}
		}
	}
}

int64_t game::apply_historical_population_group_to_province(const population_group_key &group_key, const int population, const province *province)
{
	if (population <= 0) {
		return 0;
	}

	province_history *province_history = province->get_history();
	province_game_data *province_game_data = province->get_game_data();

	const culture *province_culture = province_history->get_culture();

	const culture *culture = group_key.culture;
	if (culture == nullptr) {
		if (province_culture == nullptr) {
			log::log_error("Province \"" + province->get_identifier() + "\" has no culture.");
			return 0;
		}

		culture = province_culture;
	}
	assert_throw(culture != nullptr);

	const phenotype *phenotype = group_key.phenotype;
	if (phenotype == nullptr) {
		phenotype = culture->get_default_phenotype();
	}
	assert_throw(phenotype != nullptr);

	int population_unit_count = population / defines::get()->get_population_per_unit();

	const population_type *population_type = group_key.type;
	if (population_type == nullptr) {
		if (province_game_data->get_owner() != nullptr && province_game_data->get_owner()->is_tribe()) {
			population_type = culture->get_population_class_type(defines::get()->get_default_tribal_population_class());
		} else {
			centesimal_int literacy_rate = province_history->get_literacy_rate();
			if (literacy_rate == 0 && province_game_data->get_owner() != nullptr) {
				literacy_rate = province_game_data->get_owner()->get_history()->get_literacy_rate();
			}

			if (literacy_rate != 0) {
				const int literate_population_unit_count = (population_unit_count * literacy_rate / 100).to_int();
				population_unit_count -= literate_population_unit_count;

				const population_class *literate_population_class = defines::get()->get_default_literate_population_class();
				const metternich::population_type *literate_population_type = culture->get_population_class_type(literate_population_class);

				for (int i = 0; i < literate_population_unit_count; ++i) {
					province_game_data->create_population_unit(literate_population_type, culture, phenotype);
				}
			}

			const population_class *population_class = defines::get()->get_default_population_class();
			population_type = culture->get_population_class_type(population_class);
		}
	}
	assert_throw(population_type != nullptr);

	for (int i = 0; i < population_unit_count; ++i) {
		province_game_data->create_population_unit(population_type, culture, phenotype);
	}

	int64_t remaining_population = population % defines::get()->get_population_per_unit();
	remaining_population = std::max<int64_t>(0, remaining_population);

	return remaining_population;
}

void game::on_setup_finished()
{
	this->calculate_great_power_ranks();
	this->create_diplomatic_map_image();

	emit countries_changed();

	for (const country *country : this->get_countries()) {
		for (const QPoint &border_tile_pos : country->get_game_data()->get_border_tiles()) {
			map::get()->calculate_tile_country_border_directions(border_tile_pos);
		}

		emit country->game_data_changed();
	}

	emit setup_finished();
}

void game::do_turn()
{
	for (const country *country : this->get_countries()) {
		if (country == this->get_player_country()) {
			continue;
		}

		country->get_game_data()->do_ai_turn();
	}

	for (const country *country : this->get_countries()) {
		country->get_game_data()->do_turn();
	}

	this->calculate_great_power_ranks();

	this->increment_turn();
}

void game::do_turn_async()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		this->do_turn();
		co_return;
	});
}

void game::increment_turn()
{
	const QDateTime old_date = this->date;
	this->date = old_date.addMonths(defines::get()->get_months_per_turn());
	assert_throw(this->date != old_date);

	++this->turn;

	emit turn_changed();
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

void game::remove_country(country *country)
{
	std::erase(this->countries, country);

	if (country->is_great_power()) {
		std::erase(this->great_powers, country);
	}

	for (const metternich::country *other_country : this->get_countries()) {
		country_game_data *other_country_game_data = other_country->get_game_data();

		if (other_country_game_data->get_diplomacy_state(country) != diplomacy_state::peace) {
			other_country_game_data->set_diplomacy_state(country, diplomacy_state::peace);
		}

		if (other_country_game_data->get_consulate(country) != nullptr) {
			other_country_game_data->set_consulate(country, nullptr);
		}
	}

	if (this->is_running()) {
		emit countries_changed();
	}

	country->reset_game_data();
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
