#include "metternich.h"

#include "game/game.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_history.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "country/culture_history.h"
#include "country/diplomacy_state.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "database/preferences.h"
#include "game/game_rules.h"
#include "game/scenario.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/pathway.h"
#include "infrastructure/provincial_building_slot.h"
#include "infrastructure/wonder.h"
#include "map/direction.h"
#include "map/map.h"
#include "map/map_generator.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/route.h"
#include "map/route_game_data.h"
#include "map/route_history.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/effect/delayed_effect_instance.h"
#include "time/era.h"
#include "unit/civilian_unit.h"
#include "unit/historical_civilian_unit.h"
#include "unit/historical_civilian_unit_history.h"
#include "unit/historical_military_unit.h"
#include "unit/historical_military_unit_history.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/event_loop.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/size_util.h"
#include "util/thread_pool.h"

#include "xbrz.h"

namespace metternich {

QDateTime game::normalize_date(const QDateTime &date)
{
	QDateTime normalized_date = date;
	normalized_date.setTime(QTime(12, 0, 0));

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

game::~game()
{
}

void game::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();

	throw std::runtime_error("Invalid game data property: \"" + key + "\".");
}

void game::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "rules") {
		auto rules = make_qunique<game_rules>();
		database::process_gsml_data(rules, scope);
		this->rules = std::move(rules);
	} else if (tag == "character_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const character>>();
			database::process_gsml_data(delayed_effect, delayed_effect_data);
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "country_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const country>>();
			database::process_gsml_data(delayed_effect, delayed_effect_data);
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "province_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const province>>();
			database::process_gsml_data(delayed_effect, delayed_effect_data);
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else {
		throw std::runtime_error("Invalid game data scope: \"" + scope.get_tag() + "\".");
	}
}

gsml_data game::to_gsml_data() const
{
	gsml_data data;

	data.add_child("rules", this->get_rules()->to_gsml_data());

	if (!this->character_delayed_effects.empty()) {
		gsml_data delayed_effects_data("character_delayed_effects");
		for (const auto &delayed_effect : this->character_delayed_effects) {
			delayed_effects_data.add_child(delayed_effect->to_gsml_data());
		}
		data.add_child(std::move(delayed_effects_data));
	}

	if (!this->country_delayed_effects.empty()) {
		gsml_data delayed_effects_data("country_delayed_effects");
		for (const auto &delayed_effect : this->country_delayed_effects) {
			delayed_effects_data.add_child(delayed_effect->to_gsml_data());
		}
		data.add_child(std::move(delayed_effects_data));
	}

	if (!this->province_delayed_effects.empty()) {
		gsml_data delayed_effects_data("province_delayed_effects");
		for (const auto &delayed_effect : this->province_delayed_effects) {
			delayed_effects_data.add_child(delayed_effect->to_gsml_data());
		}
		data.add_child(std::move(delayed_effects_data));
	}

	return data;
}

void game::create_random_map(const QSize &map_size, metternich::era *era)
{
	try {
		this->clear();
		this->rules = preferences::get()->get_game_rules()->duplicate();

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
		this->rules = preferences::get()->get_game_rules()->duplicate();
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
		co_await this->create_exploration_diplomatic_map_image();

		for (const site *site : site::get_all()) {
			if (!site->get_game_data()->is_on_map()) {
				continue;
			}

			site->get_game_data()->calculate_commodity_outputs();
		}

		for (const country *country : this->get_countries()) {
			country_game_data *country_game_data = country->get_game_data();

			for (const qunique_ptr<population_unit> &population_unit : country_game_data->get_population_units()) {
				population_unit->choose_ideology();
			}

			country_game_data->check_ruler();

			//setup journal entries, marking the ones for which the country already fulfills conditions as finished, but without doing the effects
			country_game_data->check_journal_entries(true);
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
		this->set_player_country(nullptr);
	});
}

void game::clear()
{
	try {
		this->clear_delayed_effects();

		for (const province *province : province::get_all()) {
			province_game_data *province_game_data = province->get_game_data();
			province_game_data->reset_non_map_data();
		}

		for (const site *site : site::get_all()) {
			site->get_game_data()->reset_non_map_data();
		}

		//clear data related to the game (i.e. the data determined by history), but not that related only to the map
		//this is so that game setup speed can be faster if changing from one scenario to another with the same map template
		for (country *country : country::get_all()) {
			country->reset_game_data();
		}

		for (character *character : character::get_all()) {
			character->reset_game_data();
		}

		map::get()->clear_tile_game_data();

		this->scenario = nullptr;
		this->countries.clear();

		this->date = game::normalize_date(defines::get()->get_default_start_date());
		this->turn = 1;

		this->rules.reset();
		this->exploration_diplomatic_map_image = QImage();
		this->exploration_changed = false;
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to clear the game."));
	}
}

void game::apply_history(const metternich::scenario *scenario)
{
	try {
		this->date = game::normalize_date(scenario->get_start_date());

		database::get()->load_history(scenario->get_start_date(), scenario->get_timeline(), this->get_rules());

		for (const province *province : map::get()->get_provinces()) {
			try {
				const province_history *province_history = province->get_history();
				province_game_data *province_game_data = province->get_game_data();

				province_game_data->set_culture(province_history->get_culture());
				province_game_data->set_religion(province_history->get_religion());

				const country *owner = province_history->get_owner();

				province_game_data->set_owner(owner);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to apply history for province \"" + province->get_identifier() + "\"."));
			}
		}

		for (const country *country : this->get_countries()) {
			const country_history *country_history = country->get_history();
			country_game_data *country_game_data = country->get_game_data();

			if (country_history->get_religion() != nullptr) {
				country_game_data->set_religion(country_history->get_religion());
			} else {
				country_game_data->set_religion(country->get_default_religion());
			}

			const character *ruler = country_history->get_ruler();
			if (ruler != nullptr) {
				character_game_data *ruler_game_data = ruler->get_game_data();

				if (ruler_game_data->get_country() != nullptr && ruler_game_data->get_country() != country) {
					throw std::runtime_error(std::format("Cannot set \"{}\" as the ruler of \"{}\", as it is already assigned to another country.", ruler->get_identifier(), country->get_identifier()));
				}

				country_game_data->set_ruler(country_history->get_ruler());

				if (ruler->get_required_technology() != nullptr) {
					country_game_data->add_technology_with_prerequisites(ruler->get_required_technology());
				}
			}

			for (const technology *technology : country_history->get_technologies()) {
				country_game_data->add_technology_with_prerequisites(technology);
			}

			if (this->get_rules()->are_advisors_enabled() && country_game_data->can_have_advisors()) {
				for (const character *advisor : country_history->get_advisors()) {
					//add prerequisites for the advisor to its country's researched technologies
					if (advisor->get_required_technology() != nullptr) {
						country_game_data->add_technology_with_prerequisites(advisor->get_required_technology());
					}

					country_game_data->add_advisor(advisor);
				}
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

		for (const culture *culture : culture::get_all()) {
			const culture_history *culture_history = culture->get_history();

			for (const country *country : this->get_countries()) {
				if (country->get_culture() != culture) {
					continue;
				}

				culture_history->apply_to_country(country);
			}
		}

		for (const cultural_group *cultural_group : cultural_group::get_all()) {
			const culture_history *culture_history = cultural_group->get_history();

			for (const country *country : this->get_countries()) {
				if (!country->get_culture()->is_part_of_group(cultural_group)) {
					continue;
				}

				culture_history->apply_to_country(country);
			}
		}

		for (const site *site : site::get_all()) {
			const site_game_data *site_game_data = site->get_game_data();
			tile *tile = site_game_data->get_tile();

			const province *site_province = site->get_province();
			if (site_province == nullptr && tile != nullptr) {
				site_province = tile->get_province();
			}

			const site_history *site_history = site->get_history();

			//apply site buildings to its province's owner
			if (site_province != nullptr && site_province->get_game_data()->is_on_map()) {
				province_game_data *site_province_game_data = site_province->get_game_data();
				const country *owner = site_province_game_data->get_owner();

				country_game_data *owner_game_data = owner ? owner->get_game_data() : nullptr;

				for (auto [building_slot_type, building] : site_history->get_buildings()) {
					while (building != nullptr) {
						if (building->get_conditions() != nullptr) {
							if (owner == nullptr) {
								building = building->get_required_building();
								continue;
							}

							if (!building->get_conditions()->check(owner, read_only_context(owner))) {
								building = building->get_required_building();
								continue;
							}
						}

						if (building->get_province_conditions() != nullptr) {
							if (!building->get_province_conditions()->check(site_province, read_only_context(site_province))) {
								building = building->get_required_building();
								continue;
							}
						}

						//checks successful
						break;
					}

					if (building == nullptr) {
						continue;
					}

					const building_type *slot_building = nullptr;

					if (building->is_provincial()) {
						slot_building = site_province_game_data->get_slot_building(building_slot_type);
					} else {
						if (owner == nullptr) {
							continue;
						}

						slot_building = owner_game_data->get_slot_building(building_slot_type);
					}

					if (slot_building == nullptr || slot_building->get_score() < building->get_score()) {
						if (building->is_provincial()) {
							site_province_game_data->set_slot_building(building_slot_type, building);
						} else {
							owner_game_data->set_slot_building(building_slot_type, building);
						}
					}

					if (building->get_required_technology() != nullptr && owner_game_data != nullptr) {
						owner_game_data->add_technology_with_prerequisites(building->get_required_technology());
					}
				}

				for (auto [building_slot_type, wonder] : site_history->get_wonders()) {
					if (wonder->get_conditions() != nullptr) {
						if (owner == nullptr) {
							continue;
						}

						if (!wonder->get_conditions()->check(owner, read_only_context(owner))) {
							continue;
						}
					}

					if (wonder->get_province_conditions() != nullptr) {
						if (!wonder->get_province_conditions()->check(site_province, read_only_context(site_province))) {
							continue;
						}
					}

					provincial_building_slot *building_slot = site_province_game_data->get_building_slot(building_slot_type);

					if (building_slot != nullptr) {
						const metternich::wonder *slot_wonder = building_slot->get_wonder();

						if (slot_wonder == nullptr || slot_wonder->get_score() < wonder->get_score()) {
							building_slot->set_wonder(wonder);
						}
					}

					if (wonder->get_required_technology() != nullptr && owner_game_data != nullptr) {
						owner_game_data->add_technology_with_prerequisites(wonder->get_required_technology());
					}
				}
			}

			if (tile == nullptr) {
				continue;
			}

			const improvement *site_improvement = site_history->get_improvement();
			if (site_improvement == nullptr && site_history->is_developed() && site->get_type() == site_type::resource) {
				//if the site is marked as developed, but has no specific improvement set for it, pick the most basic improvement for its resource
				for (const improvement *improvement : improvement::get_all()) {
					if (improvement->get_resource() != site->get_resource()) {
						continue;
					}

					if (improvement->get_required_improvement() != nullptr) {
						continue;
					}

					site_improvement = improvement;
					break;
				}
			}

			if (site_improvement != nullptr) {
				assert_throw(site_improvement->get_resource() != nullptr || site_improvement->is_ruins());

				if (site_improvement->get_resource() != nullptr) {
					assert_throw(site->get_type() == site_type::resource);

					if (tile->get_resource() == nullptr) {
						throw std::runtime_error("Failed to set resource improvement for tile for resource site \"" + site->get_identifier() + "\", as it has no resource.");
					}

					if (tile->get_resource() != site_improvement->get_resource()) {
						throw std::runtime_error("Failed to set resource improvement for tile for resource site \"" + site->get_identifier() + "\", as its resource is different than that of the improvement.");
					}

					map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
				}

				tile->set_improvement(site_improvement);

				//add prerequisites for the tile's improvement to its owner's researched technologies
				if (tile->get_improvement()->get_required_technology() != nullptr && tile->get_owner() != nullptr) {
					tile->get_owner()->get_game_data()->add_technology_with_prerequisites(tile->get_improvement()->get_required_technology());
				}

				if (tile->get_improvement() != nullptr && tile->get_province() != nullptr) {
					tile->get_province()->get_game_data()->on_improvement_gained(tile->get_improvement(), 1);
				}
			}

			if (site_history->is_resource_discovered()) {
				assert_throw(site->get_type() == site_type::resource);
				map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
			}
		}

		for (const route *route : route::get_all()) {
			const route_game_data *route_game_data = route->get_game_data();

			if (!route_game_data->is_on_map()) {
				continue;
			}

			const route_history *route_history = route->get_history();

			const pathway *route_pathway = route_history->get_pathway();
			if (route_pathway != nullptr) {
				for (const QPoint &tile_pos : route_game_data->get_tiles()) {
					tile *tile = map::get()->get_tile(tile_pos);

					static constexpr size_t direction_count = static_cast<size_t>(direction::count);
					for (size_t i = 0; i < direction_count; ++i) {
						const direction direction = static_cast<archimedes::direction>(i);
						const pathway *direction_pathway = tile->get_direction_pathway(direction);

						if (direction_pathway != nullptr && direction_pathway->get_transport_level() < route_pathway->get_transport_level()) {
							tile->set_direction_pathway(direction, route_pathway);

							if (tile->has_river() && tile->get_owner() != nullptr && route_pathway->get_river_crossing_required_technology() != nullptr && tile->is_river_crossing_direction(direction)) {
								tile->get_owner()->get_game_data()->add_technology_with_prerequisites(route_pathway->get_river_crossing_required_technology());
							}
						}
					}

					tile->calculate_pathway_frames();

					//add prerequisites for the tile's pathway to its owner's researched technologies
					if (tile->get_owner() != nullptr) {
						if (route_pathway->get_required_technology() != nullptr) {
							tile->get_owner()->get_game_data()->add_technology_with_prerequisites(route_pathway->get_required_technology());
						}

						const technology *terrain_required_technology = route_pathway->get_terrain_required_technology(tile->get_terrain());
						if (terrain_required_technology != nullptr) {
							tile->get_owner()->get_game_data()->add_technology_with_prerequisites(terrain_required_technology);
						}
					}
				}
			}
		}

		this->apply_population_history();

		for (const character *character : character::get_all()) {
			character_game_data *character_game_data = character->get_game_data();
			const character_history *character_history = character->get_history();

			const country *country = character_history->get_country();

			if (character->get_military_unit_category() != military_unit_category::none && scenario->get_start_date() < character->get_end_date()) {
				if (character_history->get_deployment_province() != nullptr) {
					assert_throw(country != nullptr);
					character_game_data->set_country(country);

					assert_throw(character_game_data->get_country() != nullptr);
					character_game_data->deploy_to_province(character_history->get_deployment_province());
				}
			}

			country_game_data *country_game_data = country ? country->get_game_data() : nullptr;

			if (character->is_advisor() && country != nullptr && this->get_rules()->are_advisors_enabled() && country_game_data->can_have_advisors()) {
				const technology *obsolescence_technology = character->get_obsolescence_technology();

				if (obsolescence_technology != nullptr && country_game_data->has_technology(obsolescence_technology)) {
					character_game_data->set_dead(true);
				} else {
					country_game_data->add_advisor(character);
				}

				if (character->get_required_technology() != nullptr) {
					country_game_data->add_technology_with_prerequisites(character->get_required_technology());
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

			const religion *religion = historical_civilian_unit->get_religion();
			if (religion == nullptr) {
				if (home_province->get_game_data()->get_religion() != nullptr) {
					religion = home_province->get_game_data()->get_religion();
				} else {
					religion = owner->get_game_data()->get_religion();
				}
			}
			assert_throw(religion != nullptr);

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

			auto civilian_unit = make_qunique<metternich::civilian_unit>(historical_civilian_unit->get_type(), owner, population_type, culture, religion, phenotype);
			civilian_unit->set_tile_pos(tile_pos);

			owner->get_game_data()->add_civilian_unit(std::move(civilian_unit));
		}

		for (const historical_military_unit *historical_military_unit : historical_military_unit::get_all()) {
			const historical_military_unit_history *historical_military_unit_history = historical_military_unit->get_history();

			if (!historical_military_unit_history->is_active()) {
				continue;
			}

			const province *province = historical_military_unit_history->get_province();

			assert_throw(province != nullptr);

			if (!province->get_game_data()->is_on_map()) {
				continue;
			}

			const country *owner = historical_military_unit->get_owner();

			if (owner == nullptr) {
				owner = province->get_game_data()->get_owner();
			}

			assert_throw(owner != nullptr);
			assert_throw(owner->get_game_data()->is_alive());

			const metternich::province *home_province = historical_military_unit->get_home_province();
			if (home_province == nullptr) {
				if (province->get_game_data()->get_owner() == owner) {
					home_province = province;
				} else if (!owner->get_game_data()->is_under_anarchy()) {
					home_province = owner->get_capital_province();
				} else {
					continue;
				}
			}
			assert_throw(home_province != nullptr);

			const culture *culture = historical_military_unit->get_culture();
			if (culture == nullptr) {
				if (home_province->get_game_data()->get_culture() != nullptr) {
					culture = home_province->get_game_data()->get_culture();
				} else {
					culture = owner->get_culture();
				}
			}
			assert_throw(culture != nullptr);

			const religion *religion = historical_military_unit->get_religion();
			if (religion == nullptr) {
				if (home_province->get_game_data()->get_religion() != nullptr) {
					religion = home_province->get_game_data()->get_religion();
				} else {
					religion = owner->get_game_data()->get_religion();
				}
			}
			assert_throw(religion != nullptr);

			const population_type *population_type = historical_military_unit->get_population_type();
			if (population_type == nullptr) {
				population_type = culture->get_population_class_type(defines::get()->get_default_population_class());
			}
			assert_throw(population_type != nullptr);

			const phenotype *phenotype = historical_military_unit->get_phenotype();
			if (phenotype == nullptr) {
				phenotype = culture->get_default_phenotype();
			}
			assert_throw(phenotype != nullptr);

			auto military_unit = make_qunique<metternich::military_unit>(historical_military_unit->get_type(), owner, population_type, culture, religion, phenotype);
			military_unit->set_province(province);

			owner->get_game_data()->add_military_unit(std::move(military_unit));
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to apply history for scenario \"" + scenario->get_identifier() + "\"."));
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
				population_groups[population_group_key(group_key.type, group_key.culture, nullptr, nullptr)];
				population_groups[population_group_key(group_key.type, nullptr, group_key.religion, nullptr)];
				population_groups[population_group_key(group_key.type, nullptr, nullptr, group_key.phenotype)];
				population_groups[population_group_key(group_key.type, nullptr, nullptr, nullptr)];
			}

			if (group_key.culture != nullptr) {
				population_groups[population_group_key(group_key.type, group_key.culture, nullptr, nullptr)];
				population_groups[population_group_key(nullptr, group_key.culture, group_key.religion, nullptr)];
				population_groups[population_group_key(nullptr, group_key.culture, nullptr, group_key.phenotype)];
				population_groups[population_group_key(nullptr, group_key.culture, nullptr, nullptr)];
			}

			if (group_key.religion != nullptr) {
				population_groups[population_group_key(group_key.type, nullptr, group_key.religion, nullptr)];
				population_groups[population_group_key(nullptr, group_key.culture, group_key.religion, nullptr)];
				population_groups[population_group_key(nullptr, nullptr, group_key.religion, group_key.phenotype)];
				population_groups[population_group_key(nullptr, nullptr, group_key.religion, nullptr)];
			}

			if (group_key.phenotype != nullptr) {
				population_groups[population_group_key(group_key.type, nullptr, nullptr, group_key.phenotype)];
				population_groups[population_group_key(nullptr, group_key.culture, nullptr, group_key.phenotype)];
				population_groups[population_group_key(nullptr, nullptr, group_key.religion, group_key.phenotype)];
				population_groups[population_group_key(nullptr, nullptr, nullptr, group_key.phenotype)];
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
				} else if (group_key.religion != nullptr) {
					group_key_copy.religion = nullptr;
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
	const country *country = province_game_data->get_owner();

	if (country == nullptr) {
		return 0;
	}

	country_game_data *country_game_data = country->get_game_data();

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

	const religion *province_religion = province_history->get_religion();

	const religion *religion = group_key.religion;
	if (religion == nullptr) {
		if (province_religion == nullptr) {
			log::log_error("Province \"" + province->get_identifier() + "\" has no religion.");
			return 0;
		}

		religion = province_religion;
	}
	assert_throw(religion != nullptr);

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
					country_game_data->create_population_unit(literate_population_type, culture, religion, phenotype);
				}
			}

			const population_class *population_class = defines::get()->get_default_population_class();
			population_type = culture->get_population_class_type(population_class);
		}
	}
	assert_throw(population_type != nullptr);

	for (int i = 0; i < population_unit_count; ++i) {
		country_game_data->create_population_unit(population_type, culture, religion, phenotype);
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
		country->get_game_data()->check_ruler();

		for (const QPoint &border_tile_pos : country->get_game_data()->get_border_tiles()) {
			map::get()->calculate_tile_country_border_directions(border_tile_pos);
		}

		emit country->game_data_changed();
	}

	for (const character *character : character::get_all()) {
		character->get_game_data()->on_game_started();

		emit character->game_data_changed();
	}

	emit setup_finished();
}

void game::do_turn()
{
	try {
		this->process_delayed_effects();

		for (const country *country : this->get_countries()) {
			if (country->get_game_data()->is_ai()) {
				country->get_game_data()->do_ai_turn();
			}
		}

		for (const country *country : this->get_countries()) {
			country->get_game_data()->do_turn();
		}

		if (this->exploration_changed) {
			thread_pool::get()->co_spawn_sync([this]() -> boost::asio::awaitable<void> {
				co_await this->create_exploration_diplomatic_map_image();
			});
			this->exploration_changed = false;
		}

		this->calculate_great_power_ranks();

		this->increment_turn();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to process turn."));
	}
}

void game::do_turn_async()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		try {
			this->do_turn();
			co_return;
		} catch (const std::exception &exception) {
			exception::report(exception);
			std::terminate();
		}
	});
}

QDateTime game::get_next_date() const
{
	return this->get_date().addMonths(defines::get()->get_months_per_turn());
}

void game::increment_turn()
{
	const QDateTime old_date = this->get_date();
	this->date = this->get_next_date();
	assert_throw(this->get_date() != old_date);

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

	country->get_game_data()->clear_advisors();

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

boost::asio::awaitable<void> game::create_exploration_diplomatic_map_image()
{
	const map *map = map::get();

	const int tile_pixel_size = map->get_diplomatic_map_tile_pixel_size();

	this->exploration_diplomatic_map_image = QImage(map->get_size(), QImage::Format_RGBA8888);
	this->exploration_diplomatic_map_image.fill(Qt::transparent);

	const QColor &color = defines::get()->get_unexplored_terrain()->get_color();

	const country_game_data *country_game_data = this->get_player_country()->get_game_data();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos = QPoint(x, y);

			if (country_game_data->is_tile_explored(tile_pos)) {
				continue;
			}

			this->exploration_diplomatic_map_image.setPixelColor(tile_pos, color);
		}
	}

	QImage scaled_exploration_diplomatic_map_image;

	const centesimal_int final_scale_factor = tile_pixel_size * preferences::get()->get_scale_factor();

	co_await thread_pool::get()->co_spawn_awaitable([this, final_scale_factor, &scaled_exploration_diplomatic_map_image]() -> boost::asio::awaitable<void> {
		scaled_exploration_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->exploration_diplomatic_map_image, final_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->exploration_diplomatic_map_image = std::move(scaled_exploration_diplomatic_map_image);
}

void game::process_delayed_effects()
{
	this->process_delayed_effects(this->character_delayed_effects);
	this->process_delayed_effects(this->country_delayed_effects);
	this->process_delayed_effects(this->province_delayed_effects);
}

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<const character>> &&delayed_effect)
{
	this->character_delayed_effects.push_back(std::move(delayed_effect));
}

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<const country>> &&delayed_effect)
{
	this->country_delayed_effects.push_back(std::move(delayed_effect));
}

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<const province>> &&delayed_effect)
{
	this->province_delayed_effects.push_back(std::move(delayed_effect));
}

void game::clear_delayed_effects()
{
	this->character_delayed_effects.clear();
	this->country_delayed_effects.clear();
	this->province_delayed_effects.clear();
}

bool game::do_battle(const std::vector<military_unit *> &attacker_units, const std::vector<military_unit *> &defender_units)
{
	//this function returns true if the attackers won, or false otherwise

	const int attacker_score = military_unit::get_army_score(attacker_units);
	const int defender_score = military_unit::get_army_score(defender_units);

	if (attacker_score > defender_score) {
		//destroy the defender units
		for (military_unit *military_unit : defender_units) {
			military_unit->disband(false);
		}

		return true;
	}

	if (attacker_score < defender_score) {
		//destroy the attacker units
		for (military_unit *military_unit : attacker_units) {
			military_unit->disband(false);
		}
	}

	return false;
}

}
