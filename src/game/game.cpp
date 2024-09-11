#include "metternich.h"

#include "game/game.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_history.h"
#include "character/character_role.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/country_rank.h"
#include "country/country_tier.h"
#include "country/country_turn_data.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "country/culture_history.h"
#include "country/diplomacy_state.h"
#include "country/government_type.h"
#include "country/religion.h"
#include "country/tradition.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "game/game_rules.h"
#include "game/scenario.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/improvement_slot.h"
#include "infrastructure/pathway.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/settlement_type.h"
#include "infrastructure/wonder.h"
#include "map/direction.h"
#include "map/map.h"
#include "map/map_generator.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/region.h"
#include "map/region_history.h"
#include "map/route.h"
#include "map/route_game_data.h"
#include "map/route_history.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/effect/delayed_effect_instance.h"
#include "time/era.h"
#include "unit/army.h"
#include "unit/civilian_unit.h"
#include "unit/civilian_unit_type.h"
#include "unit/historical_civilian_unit.h"
#include "unit/historical_civilian_unit_history.h"
#include "unit/historical_military_unit.h"
#include "unit/historical_military_unit_history.h"
#include "unit/historical_transporter.h"
#include "unit/historical_transporter_history.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_type.h"
#include "unit/transporter.h"
#include "unit/transporter_type.h"
#include "util/aggregate_exception.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/date_util.h"
#include "util/exception_util.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/size_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

#include "xbrz.h"

namespace metternich {

QDate game::normalize_date(const QDate &date)
{
	QDate normalized_date = date;

	if (normalized_date.day() != 1) {
		normalized_date.setDate(normalized_date.year(), normalized_date.month(), 1);
	}

	const int months_per_turn = defines::get()->get_months_per_turn(date.year());

	const int month_rest = (normalized_date.month() - 1) % months_per_turn;
	if (month_rest != 0) {
		normalized_date.setDate(normalized_date.year(), normalized_date.month() - month_rest, normalized_date.day());
	}

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

QCoro::Task<void> game::create_random_map_coro(const QSize map_size, metternich::era *era)
{
	try {
		this->clear();
		this->rules = preferences::get()->get_game_rules()->duplicate();

		map_generator map_generator(map_size, era);
		map_generator.generate();

		this->date = game::normalize_date(era->get_start_date());

		co_await this->on_setup_finished();
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
	}
}

QCoro::Task<void> game::setup_scenario_coro(metternich::scenario *scenario)
{
	try {
		const metternich::scenario *old_scenario = this->scenario;

		this->clear();
		this->rules = preferences::get()->get_game_rules()->duplicate();
		this->scenario = scenario;

		if (old_scenario == nullptr || old_scenario->get_map_template() != scenario->get_map_template()) {
			scenario->get_map_template()->apply();
			map::get()->initialize();

			//reset the game data for provinces and sites, since their constructors rely on the map having been initialized before
			this->reset_game_data();
		}

		this->apply_history(scenario);
		co_await this->on_setup_finished();
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
	}
}

QCoro::Task<void> game::start_coro()
{
	if (this->is_running()) {
		//already running
		co_return;
	}

	map::get()->create_minimap_image();
	co_await this->create_exploration_diplomatic_map_image();

	this->adjust_food_production_for_country_populations();

	for (const site *site : site::get_all()) {
		if (!site->get_game_data()->is_on_map()) {
			continue;
		}

		site->get_game_data()->check_settlement_type();
		site->get_game_data()->calculate_commodity_outputs();
	}

	for (const country *country : this->get_countries()) {
		country_game_data *country_game_data = country->get_game_data();

		for (const province *province : country_game_data->get_provinces()) {
			province->get_game_data()->check_employment();
		}

		for (population_unit *population_unit : country_game_data->get_population_units()) {
			population_unit->choose_ideology();
		}

		country_game_data->check_ruler();

		//setup journal entries, marking the ones for which the country already fulfills conditions as finished, but without doing the effects
		country_game_data->check_journal_entries(true, true);

		//assign transport orders for countries on start
		country_game_data->assign_transport_orders();
	}

	this->set_running(true);
}

void game::stop()
{
	if (!this->is_running()) {
		//already stopped
		return;
	}

	this->set_running(false);
	this->clear();
	map::get()->clear();
	this->set_player_country(nullptr);
}

void game::clear()
{
	try {
		this->clear_delayed_effects();

		this->reset_game_data();

		map::get()->clear_tile_game_data();

		this->scenario = nullptr;
		this->countries.clear();
		this->great_powers.clear();
		this->prices.clear();

		this->date = game::normalize_date(defines::get()->get_default_start_date());
		this->turn = 1;

		this->rules.reset();
		this->exploration_diplomatic_map_image = QImage();
		this->exploration_changed = false;
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to clear the game."));
	}
}

void game::reset_game_data()
{
	//clear data related to the game (i.e. the data determined by history), but not that related only to the map
	//this is so that game setup speed can be faster if changing from one scenario to another with the same map template
	for (province *province : province::get_all()) {
		province->reset_game_data();
	}

	for (site *site : site::get_all()) {
		site->reset_game_data();
	}

	for (country *country : country::get_all()) {
		country->reset_game_data();
	}

	for (character *character : character::get_all()) {
		character->reset_game_data();
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

				if (province->is_water_zone()) {
					continue;
				}

				province_game_data->set_culture(province_history->get_culture());
				province_game_data->set_religion(province_history->get_religion());

				const country *owner = province_history->get_owner();
				province_game_data->set_owner(owner);

				if (owner == nullptr) {
					log::log_error(std::format("Province \"{}\" has no owner for scenario \"{}\".", province->get_identifier(), scenario->get_identifier()));
				}
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to apply history for province \"" + province->get_identifier() + "\"."));
			}
		}

		for (const country *country : this->get_countries()) {
			const country_history *country_history = country->get_history();
			country_game_data *country_game_data = country->get_game_data();

			if (country_history->get_tier() != country_tier::none) {
				country->get_game_data()->set_tier(country_history->get_tier());
			}

			if (country_history->get_religion() != nullptr) {
				country_game_data->set_religion(country_history->get_religion());
			}

			const subject_type *subject_type = country_history->get_subject_type();
			if (subject_type != nullptr) {
				country_game_data->set_subject_type(subject_type);
			}

			const character *ruler = country_history->get_ruler();
			if (ruler != nullptr) {
				character_game_data *ruler_game_data = ruler->get_game_data();

				if (ruler_game_data->get_country() != nullptr && ruler_game_data->get_country() != country) {
					throw std::runtime_error(std::format("Cannot set \"{}\" as the ruler of \"{}\", as it is already assigned to another country.", ruler->get_identifier(), country->get_identifier()));
				}

				country_game_data->set_ruler(ruler);

				if (ruler->get_required_technology() != nullptr) {
					country_game_data->add_technology_with_prerequisites(ruler->get_required_technology());
				}
			}

			for (const technology *technology : country_history->get_technologies()) {
				country_game_data->add_technology_with_prerequisites(technology);
			}

			for (const tradition *tradition : country_history->get_traditions()) {
				country_game_data->gain_tradition_with_prerequisites(tradition);
			}

			country_game_data->set_wealth(country_history->get_wealth());

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

		for (const country *country : this->get_countries()) {
			country_game_data *country_game_data = country->get_game_data();

			if (country_game_data->get_overlord() != nullptr) {
				if (country_game_data->get_subject_type() == nullptr) {
					throw std::runtime_error(std::format("Country \"{}\" is a vassal, but has no subject type.", country->get_identifier()));
				}
			} else {
				if (country_game_data->get_subject_type() != nullptr) {
					log::log_error(std::format("Country \"{}\" is not a vassal, but has a subject type.", country->get_identifier()));

					country_game_data->set_subject_type(nullptr);
				}
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

		this->apply_sites();

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

		for (const country *country : this->get_countries()) {
			country->get_game_data()->set_capital(nullptr);
			country->get_game_data()->choose_capital();
		}

		this->apply_population_history();

		for (const province *province : map::get()->get_provinces()) {
			for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
				site_game_data *settlement_game_data = settlement->get_game_data();
				if (settlement_game_data->is_built()) {
					settlement_game_data->check_free_buildings();
				}
			}
		}

		//set stored commodities from history after the initial buildings have been constructed, so that buildings granting storage capacity (e.g. warehouses) will already be present
		for (const country *country : this->get_countries()) {
			for (const auto &[commodity, quantity] : country->get_history()->get_commodities()) {
				country->get_game_data()->set_stored_commodity(commodity, quantity);
			}
		}

		for (const character *character : character::get_all()) {
			character_game_data *character_game_data = character->get_game_data();
			const character_history *character_history = character->get_history();

			const country *country = character_history->get_country();

			if ((character->get_role() == character_role::advisor || character->get_role() == character_role::leader || character->get_role() == character_role::civilian) && country != nullptr && !country->get_game_data()->is_under_anarchy()) {
				country_game_data *country_game_data = country->get_game_data();
				const technology *obsolescence_technology = character->get_obsolescence_technology();

				if (character->get_required_technology() != nullptr) {
					country_game_data->add_technology_with_prerequisites(character->get_required_technology());
				}

				if (obsolescence_technology != nullptr && country_game_data->has_technology(obsolescence_technology)) {
					character_game_data->set_dead(true);
				} else {
					if (character->get_role() == character_role::advisor) {
						if (country_game_data->can_have_advisors() && !country_game_data->has_incompatible_advisor_to(character)) {
							country_game_data->add_advisor(character);
						}
					} else if (character->get_role() == character_role::leader) {
						const province *deployment_province = character_history->get_deployment_province();
						if (deployment_province == nullptr && country_game_data->get_capital_province() != nullptr) {
							deployment_province = country_game_data->get_capital_province();
						}

						assert_throw(deployment_province != nullptr);

						if (deployment_province->is_water_zone() || deployment_province->get_game_data()->get_owner() == country) {
							country_game_data->add_leader(character);

							assert_throw(character_game_data->get_country() != nullptr);
							character_game_data->deploy_to_province(deployment_province);
						}
					} else if (character->get_role() == character_role::civilian) {
						const site *deployment_site = character_history->get_deployment_site();
						if (deployment_site == nullptr && country_game_data->get_capital() != nullptr) {
							deployment_site = country_game_data->get_capital();
						}

						assert_throw(deployment_site != nullptr);

						if (!deployment_site->get_game_data()->is_on_map()) {
							continue;
						}

						if (deployment_site->get_game_data()->get_owner() != country) {
							continue;
							country_game_data->add_leader(character);

							assert_throw(character_game_data->get_country() != nullptr);
							character_game_data->deploy_to_province(character_history->get_deployment_province());
						}

						const civilian_unit_type *type = character->get_civilian_unit_type();
						assert_throw(type != nullptr);

						if (type->get_required_technology() != nullptr) {
							country_game_data->add_technology_with_prerequisites(type->get_required_technology());
						}

						const QPoint tile_pos = deployment_site->get_game_data()->get_tile_pos();

						if (map::get()->get_tile(tile_pos)->get_civilian_unit() != nullptr) {
							log::log_error(std::format("Cannot deploy civilian character \"{}\" to site \"{}\", since that site's tile is already occupied by another civilian unit.", character->get_identifier(), deployment_site->get_identifier()));
							continue;
						}

						auto civilian_unit = make_qunique<metternich::civilian_unit>(character, country);
						civilian_unit->set_tile_pos(tile_pos);

						country_game_data->add_civilian_unit(std::move(civilian_unit));
					}
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

			const country *owner = historical_civilian_unit->get_owner();

			if (owner == nullptr) {
				owner = site->get_game_data()->get_owner();
			}

			assert_throw(owner != nullptr);

			country_game_data *owner_game_data = owner->get_game_data();

			assert_throw(owner_game_data->is_alive());

			const civilian_unit_type *type = historical_civilian_unit->get_type();
			assert_throw(type != nullptr);

			if (type->get_required_technology() != nullptr) {
				owner_game_data->add_technology_with_prerequisites(type->get_required_technology());
			}

			const metternich::site *home_settlement = historical_civilian_unit->get_home_settlement();
			if (home_settlement == nullptr) {
				if (site->get_game_data()->get_owner() == owner && site->is_settlement() && site->get_game_data()->is_built()) {
					home_settlement = site;
				} else if (!owner_game_data->is_under_anarchy()) {
					home_settlement = owner_game_data->get_capital();
				} else {
					continue;
				}
			}
			assert_throw(home_settlement != nullptr);

			const culture *culture = historical_civilian_unit->get_culture();
			if (culture == nullptr) {
				if (home_settlement->get_game_data()->get_culture() != nullptr) {
					culture = home_settlement->get_game_data()->get_culture();
				} else {
					culture = owner->get_culture();
				}
			}
			assert_throw(culture != nullptr);

			const religion *religion = historical_civilian_unit->get_religion();
			if (religion == nullptr) {
				if (home_settlement->get_game_data()->get_religion() != nullptr) {
					religion = home_settlement->get_game_data()->get_religion();
				} else {
					religion = owner_game_data->get_religion();
				}
			}
			assert_throw(religion != nullptr);

			const population_type *population_type = historical_civilian_unit->get_population_type();
			if (population_type == nullptr) {
				population_type = culture->get_population_class_type(owner_game_data->get_default_population_class());
			}
			assert_throw(population_type != nullptr);

			const phenotype *phenotype = historical_civilian_unit->get_phenotype();
			if (phenotype == nullptr) {
				phenotype = culture->get_default_phenotype();
			}
			assert_throw(phenotype != nullptr);

			const QPoint tile_pos = site->get_game_data()->get_tile_pos();

			if (map::get()->get_tile(tile_pos)->get_civilian_unit() != nullptr) {
				//tile already occupied
				continue;
			}

			auto civilian_unit = make_qunique<metternich::civilian_unit>(historical_civilian_unit->get_type(), owner, population_type, culture, religion, phenotype, home_settlement);
			civilian_unit->set_tile_pos(tile_pos);

			owner_game_data->add_civilian_unit(std::move(civilian_unit));
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

			const country *country = historical_military_unit->get_country();

			if (country == nullptr) {
				country = province->get_game_data()->get_owner();
			}

			assert_throw(country != nullptr);

			country_game_data *country_game_data = country->get_game_data();

			assert_throw(country_game_data->is_alive());

			const military_unit_type *type = historical_military_unit->get_type();
			assert_throw(type != nullptr);

			if (type->get_required_technology() != nullptr) {
				country_game_data->add_technology_with_prerequisites(type->get_required_technology());
			}

			const site *home_settlement = historical_military_unit->get_home_settlement();
			if (home_settlement == nullptr) {
				if (province->get_game_data()->get_owner() == country) {
					home_settlement = province->get_provincial_capital();
				} else if (!country_game_data->is_under_anarchy()) {
					home_settlement = country_game_data->get_capital();
				} else {
					continue;
				}
			}
			assert_throw(home_settlement != nullptr);

			const culture *culture = historical_military_unit->get_culture();
			if (culture == nullptr) {
				if (home_settlement->get_game_data()->get_culture() != nullptr) {
					culture = home_settlement->get_game_data()->get_culture();
				} else {
					culture = country->get_culture();
				}
			}
			assert_throw(culture != nullptr);

			const religion *religion = historical_military_unit->get_religion();
			if (religion == nullptr) {
				if (home_settlement->get_game_data()->get_religion() != nullptr) {
					religion = home_settlement->get_game_data()->get_religion();
				} else {
					religion = country_game_data->get_religion();
				}
			}
			assert_throw(religion != nullptr);

			const population_type *population_type = historical_military_unit->get_population_type();
			if (population_type == nullptr) {
				population_type = culture->get_population_class_type(country_game_data->get_default_population_class());
			}
			assert_throw(population_type != nullptr);

			const phenotype *phenotype = historical_military_unit->get_phenotype();
			if (phenotype == nullptr) {
				phenotype = culture->get_default_phenotype();
			}
			assert_throw(phenotype != nullptr);

			for (int i = 0; i < historical_military_unit->get_quantity(); ++i) {
				auto military_unit = make_qunique<metternich::military_unit>(type, country, population_type, culture, religion, phenotype, home_settlement);
				military_unit->set_province(province);

				for (const promotion *promotion : historical_military_unit_history->get_promotions()) {
					military_unit->add_promotion(promotion);
				}

				country_game_data->add_military_unit(std::move(military_unit));
			}
		}

		for (const historical_transporter *historical_transporter : historical_transporter::get_all()) {
			const historical_transporter_history *historical_transporter_history = historical_transporter->get_history();

			if (!historical_transporter_history->is_active()) {
				continue;
			}

			const country *country = historical_transporter->get_country();
			assert_throw(country != nullptr);

			country_game_data *country_game_data = country->get_game_data();

			if (!country_game_data->is_alive()) {
				continue;
			}

			const transporter_type *type = historical_transporter->get_type();
			assert_throw(type != nullptr);

			if (type->get_required_technology() != nullptr) {
				country_game_data->add_technology_with_prerequisites(type->get_required_technology());
			}

			const site *home_settlement = historical_transporter->get_home_settlement();
			if (home_settlement == nullptr) {
				if (!country_game_data->is_under_anarchy()) {
					home_settlement = country_game_data->get_capital();
				} else {
					continue;
				}
			}
			assert_throw(home_settlement != nullptr);

			const culture *culture = historical_transporter->get_culture();
			if (culture == nullptr) {
				if (home_settlement->get_game_data()->get_culture() != nullptr) {
					culture = home_settlement->get_game_data()->get_culture();
				} else {
					culture = country->get_culture();
				}
			}
			assert_throw(culture != nullptr);

			const religion *religion = historical_transporter->get_religion();
			if (religion == nullptr) {
				if (home_settlement->get_game_data()->get_religion() != nullptr) {
					religion = home_settlement->get_game_data()->get_religion();
				} else {
					religion = country_game_data->get_religion();
				}
			}
			assert_throw(religion != nullptr);

			const population_type *population_type = historical_transporter->get_population_type();
			if (population_type == nullptr) {
				population_type = culture->get_population_class_type(country_game_data->get_default_population_class());
			}
			assert_throw(population_type != nullptr);

			const phenotype *phenotype = historical_transporter->get_phenotype();
			if (phenotype == nullptr) {
				phenotype = culture->get_default_phenotype();
			}
			assert_throw(phenotype != nullptr);

			for (int i = 0; i < historical_transporter->get_quantity(); ++i) {
				auto transporter = make_qunique<metternich::transporter>(type, country, population_type, culture, religion, phenotype, home_settlement);

				country_game_data->add_transporter(std::move(transporter));
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to apply history for scenario \"" + scenario->get_identifier() + "\"."));
	}
}

void game::apply_sites()
{
	for (const site *site : site::get_all()) {
		site_game_data *site_game_data = site->get_game_data();
		const tile *tile = site_game_data->get_tile();

		if (tile == nullptr) {
			continue;
		}

		const province *site_province = site->get_province();
		if (site_province == nullptr) {
			site_province = tile->get_province();
		}

		if (site_province != nullptr && site_province->get_game_data()->is_on_map()) {
			const site_history *site_history = site->get_history();

			if (site_history->get_settlement_type() != nullptr) {
				if (!site->is_settlement()) {
					throw std::runtime_error(std::format("Site \"{}\" has a settlement type in history, but is not a settlement.", site->get_identifier()));
				}

				site_game_data->set_settlement_type(site_history->get_settlement_type());

				if (tile->get_resource() != nullptr) {
					map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
				}
			}
		}
	}

	//ensure provincial capitals always have a settlement
	for (const province *province : map::get()->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		const site *provincial_capital = province->get_provincial_capital();
		site_game_data *provincial_capital_game_data = provincial_capital->get_game_data();

		if (provincial_capital_game_data->is_built()) {
			continue;
		}

		const settlement_type *best_settlement_type = nullptr;
		for (const settlement_type *settlement_type : settlement_type::get_all()) {
			if (!settlement_type->get_base_settlement_types().empty()) {
				continue;
			}

			if (settlement_type->get_conditions() != nullptr && !settlement_type->get_conditions()->check(provincial_capital, read_only_context(provincial_capital))) {
				continue;
			}

			if (settlement_type->get_build_conditions() != nullptr && !settlement_type->get_build_conditions()->check(provincial_capital, read_only_context(provincial_capital))) {
				continue;
			}

			best_settlement_type = settlement_type;
			break;
		}

		assert_throw(best_settlement_type != nullptr);
		provincial_capital_game_data->set_settlement_type(best_settlement_type);
	}

	for (const site *site : site::get_all()) {
		try {
			site_game_data *site_game_data = site->get_game_data();
			tile *tile = site_game_data->get_tile();

			const province *site_province = site->get_province();
			if (site_province == nullptr && tile != nullptr) {
				site_province = tile->get_province();
			}

			const site_history *site_history = site->get_history();

			if (site_province != nullptr && site_province->get_game_data()->is_on_map()) {
				this->apply_site_buildings(site);
			}

			if (tile == nullptr) {
				continue;
			}

			std::map<improvement_slot, const improvement *> site_improvements = site_history->get_improvements();

			if (!site_improvements.contains(improvement_slot::resource) && site_history->is_developed() && site->get_map_data()->get_resource() != nullptr) {
				//if the site is marked as developed, but has no specific improvement set for it, pick the most basic improvement for its resource
				for (const improvement *improvement : site->get_map_data()->get_resource()->get_improvements()) {
					if (improvement->get_required_improvement() != nullptr) {
						continue;
					}

					site_improvements[improvement_slot::resource] = improvement;
					break;
				}
			}

			for (const auto &[improvement_slot, improvement] : site_history->get_improvements()) {
				if (improvement != nullptr) {
					if (improvement->get_resource() != nullptr) {
						assert_throw(site->get_type() == site_type::resource || site->get_type() == site_type::settlement);

						if (tile->get_resource() == nullptr) {
							throw std::runtime_error("Failed to set resource improvement for tile for site \"" + site->get_identifier() + "\", as it has no resource.");
						}

						if (tile->get_resource() != improvement->get_resource()) {
							throw std::runtime_error("Failed to set resource improvement for tile for site \"" + site->get_identifier() + "\", as its resource is different than that of the improvement.");
						}

						map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
					}

					site_game_data->set_improvement(improvement->get_slot(), improvement);

					//add prerequisites for the tile's improvement to its owner's researched technologies
					if (improvement->get_required_technology() != nullptr && tile->get_owner() != nullptr) {
						tile->get_owner()->get_game_data()->add_technology_with_prerequisites(improvement->get_required_technology());
					}
				}
			}

			if (site_history->is_resource_discovered()) {
				assert_throw(site->get_type() == site_type::resource || site->is_settlement());
				map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error(std::format("Failed to apply history for site \"{}\".", site->get_identifier())));
		}
	}
}

void game::apply_site_buildings(const site *site)
{
	site_game_data *site_game_data = site->get_game_data();
	const tile *tile = site_game_data->get_tile();

	const province *site_province = site->get_province();
	if (site_province == nullptr && tile != nullptr) {
		site_province = tile->get_province();
	}

	const metternich::site *settlement = site->is_settlement() && tile != nullptr ? site : site_province->get_provincial_capital();

	if (settlement == nullptr) {
		return;
	}

	metternich::site_game_data *settlement_game_data = settlement->get_game_data();
	const metternich::settlement_type *settlement_type = settlement_game_data->get_settlement_type();

	const site_history *site_history = site->get_history();

	if (site == settlement && settlement_game_data->is_built()) {
		//set an initial culture/religion so that buildings can be applied without issues
		if (site_history->get_culture() != nullptr) {
			settlement_game_data->set_culture(site_history->get_culture());
		} else {
			settlement_game_data->set_culture(settlement_game_data->get_province()->get_game_data()->get_culture());
		}

		if (site_history->get_religion() != nullptr) {
			settlement_game_data->set_religion(site_history->get_religion());
		} else {
			settlement_game_data->set_religion(settlement_game_data->get_province()->get_game_data()->get_religion());
		}
	}

	const country *owner = settlement_game_data->get_owner();
	country_game_data *owner_game_data = owner ? owner->get_game_data() : nullptr;

	for (auto [building_slot_type, building] : site_history->get_buildings()) {
		const settlement_building_slot *building_slot = settlement_game_data->get_building_slot(building_slot_type);

		while (building != nullptr) {
			if (building->is_provincial() && settlement == site) {
				if (settlement_type == nullptr) {
					throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have building \"{}\", but has no settlement type.", settlement->get_identifier(), building->get_identifier()));
				}

				if (!vector::contains(building->get_settlement_types(), settlement_type)) {
					throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have building \"{}\", but its settlement type of \"{}\" is not appropriate for it.", settlement->get_identifier(), building->get_identifier(), settlement_type->get_identifier()));
				}
			}

			if (building_slot->can_have_building(building)) {
				break;
			}

			building = building->get_required_building();
		}

		if (building == nullptr) {
			continue;
		}

		const building_type *slot_building = nullptr;

		if (building->is_provincial()) {
			slot_building = settlement_game_data->get_slot_building(building_slot_type);
		} else {
			if (owner == nullptr) {
				continue;
			}

			slot_building = owner_game_data->get_slot_building(building_slot_type);
		}

		if (slot_building == nullptr || slot_building->get_level() < building->get_level()) {
			if (building->is_provincial()) {
				settlement_game_data->set_slot_building(building_slot_type, building);
			} else {
				owner_game_data->set_slot_building(building_slot_type, building);
			}
		}

		if (building->get_required_technology() != nullptr && owner_game_data != nullptr) {
			owner_game_data->add_technology_with_prerequisites(building->get_required_technology());
		}
	}

	for (auto [building_slot_type, wonder] : site_history->get_wonders()) {
		if (settlement_type == nullptr) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but has no settlement type.", settlement->get_identifier(), wonder->get_identifier()));
		}

		if (!vector::contains(wonder->get_building()->get_settlement_types(), settlement_type)) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but its settlement type of \"{}\" is not appropriate for the wonder's building type of \"{}\".", settlement->get_identifier(), wonder->get_identifier(), settlement_type->get_identifier(), wonder->get_building()->get_identifier()));
		}

		settlement_building_slot *building_slot = settlement_game_data->get_building_slot(building_slot_type);

		if (building_slot == nullptr) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but does not have its building slot available.", settlement->get_identifier(), wonder->get_identifier()));
		}

		if (!building_slot->can_have_wonder(wonder)) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but cannot have it.", settlement->get_identifier(), wonder->get_identifier()));
		}

		building_slot->set_wonder(wonder);

		if (wonder->get_required_technology() != nullptr && owner_game_data != nullptr) {
			owner_game_data->add_technology_with_prerequisites(wonder->get_required_technology());
		}
	}
}

void game::apply_population_history()
{
	for (const province *province : map::get()->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		province_game_data *province_game_data = province->get_game_data();

		if (province_game_data->get_settlement_count() == 0) {
			continue;
		}

		province->get_history()->initialize_population();
	}

	//distribute region populations
	std::vector<region *> regions = region::get_all();

	std::sort(regions.begin(), regions.end(), [](const region *lhs, const region *rhs) {
		//give priority to smaller regions
		if (lhs->get_provinces().size() != rhs->get_provinces().size()) {
			return lhs->get_provinces().size() < rhs->get_provinces().size();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const region *region : regions) {
		region->get_history()->distribute_population();
	}

	country_map<population_group_map<int>> country_populations;

	for (const province *province : map::get()->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		province_game_data *province_game_data = province->get_game_data();

		if (province_game_data->get_settlement_count() == 0) {
			continue;
		}

		province->get_history()->distribute_population();

		for (const site *settlement : province_game_data->get_settlement_sites()) {
			site_game_data *settlement_game_data = settlement->get_game_data();
			site_history *settlement_history = settlement->get_history();

			settlement_history->initialize_population();

			for (const auto &[group_key, population] : settlement_history->get_population_groups()) {
				if (population <= 0) {
					continue;
				}

				const int64_t remaining_population = this->apply_historical_population_group_to_settlement(group_key, population, settlement);

				if (remaining_population != 0 && settlement_game_data->get_owner() != nullptr) {
					//add the remaining population to remaining population data for the owner
					country_populations[settlement_game_data->get_owner()][group_key] += remaining_population;
				}
			}
		}
	}

	for (auto &[country, population_groups] : country_populations) {
		const site *capital = country->get_game_data()->get_capital();

		if (capital == nullptr) {
			continue;
		}

		site_game_data *capital_game_data = capital->get_game_data();
		assert_throw(capital_game_data->get_owner() == country);

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

			const int64_t remaining_population = this->apply_historical_population_group_to_settlement(group_key, population, capital);

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

int64_t game::apply_historical_population_group_to_settlement(const population_group_key &group_key, const int population, const site *settlement)
{
	if (population <= 0) {
		return 0;
	}

	log_trace(std::format("Applying historical population group of type \"{}\", culture \"{}\", religion \"{}\" and size {} for settlement \"{}\".", group_key.type ? group_key.type->get_identifier() : "none", group_key.culture ? group_key.culture->get_identifier() : "none", group_key.religion ? group_key.religion->get_identifier() : "none", population, settlement->get_identifier()));

	site_history *settlement_history = settlement->get_history();
	site_game_data *settlement_game_data = settlement->get_game_data();

	const province *province = settlement->get_game_data()->get_province();
	province_history *province_history = province->get_history();

	const country *country = settlement_game_data->get_owner();

	if (country == nullptr) {
		return 0;
	}

	const culture *settlement_culture = settlement_history->get_culture();
	const culture *province_culture = province_history->get_culture();

	const culture *culture = group_key.culture;
	if (culture == nullptr) {
		if (settlement_culture != nullptr) {
			culture = settlement_culture;
		} else if (province_culture != nullptr) {
			culture = province_culture;
		} else {
			log::log_error(std::format("Province \"{}\" has no culture.", province->get_identifier()));
			return 0;
		}
	}
	assert_throw(culture != nullptr);

	const religion *settlement_religion = settlement_history->get_religion();
	const religion *province_religion = province_history->get_religion();

	const religion *religion = group_key.religion;
	if (religion == nullptr) {
		if (settlement_religion != nullptr) {
			religion = settlement_religion;
		} else if (province_religion != nullptr) {
			religion = province_religion;
		} else {
			log::log_error(std::format("Province \"{}\" has no religion.", province->get_identifier()));
			return 0;
		}
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
		if (!country->get_game_data()->is_tribal()) {
			centesimal_int literacy_rate = settlement_history->get_literacy_rate();
			if (literacy_rate == 0) {
				literacy_rate = province_history->get_literacy_rate();
			}
			if (literacy_rate == 0) {
				literacy_rate = country->get_history()->get_literacy_rate();
			}

			if (literacy_rate != 0) {
				const int literate_population_unit_count = (population_unit_count * literacy_rate / 100).to_int();
				population_unit_count -= literate_population_unit_count;

				const population_class *literate_population_class = defines::get()->get_default_literate_population_class();
				const metternich::population_type *literate_population_type = culture->get_population_class_type(literate_population_class);

				for (int i = 0; i < literate_population_unit_count; ++i) {
					settlement_game_data->create_population_unit(literate_population_type, culture, religion, phenotype);
				}
			}
		}

		const population_class *population_class = country->get_game_data()->get_default_population_class();
		population_type = culture->get_population_class_type(population_class);
	}
	assert_throw(population_type != nullptr);

	for (int i = 0; i < population_unit_count; ++i) {
		settlement_game_data->create_population_unit(population_type, culture, religion, phenotype);
	}

	int64_t remaining_population = population % defines::get()->get_population_per_unit();
	remaining_population = std::max<int64_t>(0, remaining_population);

	log_trace(std::format("Remaining population: {}.", remaining_population));

	return remaining_population;
}

QCoro::Task<void> game::on_setup_finished()
{
	co_await this->create_diplomatic_map_image();

	emit countries_changed();

	for (const country *country : this->get_countries()) {
		country->get_game_data()->check_government_type();
		country->get_game_data()->check_laws();
		country->get_game_data()->check_ruler();

		for (const QPoint &border_tile_pos : country->get_game_data()->get_border_tiles()) {
			map::get()->calculate_tile_country_border_directions(border_tile_pos);
		}

		//build free on start buildings
		for (const province *province : country->get_game_data()->get_provinces()) {
			for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
				if (!settlement->get_game_data()->is_built()) {
					continue;
				}

				for (const building_type *building : building_type::get_all()) {
					if (!building->is_free_on_start()) {
						continue;
					}

					settlement->get_game_data()->check_free_building(building);
				}
			}

			province->get_game_data()->check_employment();
		}

		//calculate it here rather than on start so that score is displayed properly
		country->get_game_data()->calculate_tile_transport_levels();

		emit country->game_data_changed();
	}

	this->calculate_great_power_ranks();

	for (const character *character : character::get_all()) {
		character->get_game_data()->on_setup_finished();

		emit character->game_data_changed();
	}

	emit setup_finished();
}

void game::adjust_food_production_for_country_populations()
{
	//increase food resource improvement and building levels to prevent starvation on game start to the extent possible
	for (const country *country : this->get_countries()) {
		int food_output = 0;

		for (const auto &[commodity, output] : country->get_game_data()->get_commodity_outputs()) {
			if (commodity->is_food()) {
				food_output += output.to_int();
			}
		}

		int net_food = food_output - country->get_game_data()->get_net_food_consumption();

		if (net_food >= 0) {
			continue;
		}

		static constexpr int max_resource_level = 4;

		for (int i = 1; i <= max_resource_level; ++i) {
			for (const province *province : country->get_game_data()->get_provinces()) {
				//construct food resource improvements
				for (const QPoint &resource_tile_pos : province->get_game_data()->get_resource_tiles()) {
					tile *resource_tile = map::get()->get_tile(resource_tile_pos);
					const site *resource_site = resource_tile->get_site();

					if (resource_site->get_type() != site_type::resource && resource_site->get_type() != site_type::settlement) {
						continue;
					}

					const resource *resource = resource_tile->get_resource();
					assert_throw(resource != nullptr);

					if (!resource->get_commodity()->is_food()) {
						continue;
					}

					for (const improvement *improvement : resource->get_improvements()) {
						if (improvement->get_output_multiplier() > i) {
							continue;
						}

						if (!improvement->is_buildable_on_site(resource_site)) {
							continue;
						}

						resource_site->get_game_data()->set_improvement(improvement->get_slot(), improvement);
						++net_food;
						break;
					}

					if (net_food >= 0) {
						break;
					}
				}

				if (net_food >= 0) {
					break;
				}
			}

			if (net_food >= 0) {
				break;
			}
		}
	}
}

QCoro::Task<void> game::do_turn_coro()
{
	try {
		this->process_delayed_effects();

		country_map<commodity_map<int>> old_bids;
		country_map<commodity_map<int>> old_offers;

		for (country *country : this->get_countries()) {
			country->reset_turn_data();

			country->get_game_data()->calculate_commodity_needs();

			if (country->get_game_data()->is_ai()) {
				country->get_game_data()->do_ai_turn();
			}

			old_bids[country] = country->get_game_data()->get_bids();
			old_offers[country] = country->get_game_data()->get_offers();
		}

		this->do_trade();

		for (const country *country : this->get_countries()) {
			country->get_game_data()->do_turn();
		}

		for (const country *country : this->get_countries()) {
			//do inflation after processing the normal turn for all countries, since subjects can send treasure fleets to their overlords, causing inflation in the latter
			country->get_game_data()->do_inflation();

			if (country->get_turn_data()->is_transport_level_recalculation_needed()) {
				country->get_game_data()->calculate_tile_transport_levels();
				country->get_turn_data()->set_transport_level_recalculation_needed(false);
			}
		}

		for (const country *country : this->get_countries()) {
			//do country events after processing the turn for each country, so that e.g. events won't refer to a population unit scope which no longer exists by the time the player gets to choose and option, because the population unit died due to starvation
			country->get_game_data()->do_events();

			//restore old bids and offers, if possible
			for (const auto &[commodity, bid] : old_bids[country]) {
				country->get_game_data()->set_bid(commodity, bid);
			}

			for (const auto &[commodity, offer] : old_offers[country]) {
				country->get_game_data()->set_offer(commodity, offer);
			}
		}

		for (const country *country : this->get_countries()) {
			if (country->get_turn_data()->is_diplomatic_map_dirty()) {
				co_await country->get_game_data()->create_diplomatic_map_image();
			} else {
				for (const diplomatic_map_mode mode : country->get_turn_data()->get_dirty_diplomatic_map_modes()) {
					co_await country->get_game_data()->create_diplomatic_map_mode_image(mode);
				}

				for (const diplomacy_state state : country->get_turn_data()->get_dirty_diplomatic_map_diplomacy_states()) {
					co_await country->get_game_data()->create_diplomacy_state_diplomatic_map_image(state);
				}
			}
		}

		if (this->exploration_changed) {
			co_await this->create_exploration_diplomatic_map_image();
			this->exploration_changed = false;
		}

		this->calculate_great_power_ranks();

		this->increment_turn();
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("Failed to process turn.");
		QApplication::exit(EXIT_FAILURE);
	}
}

void game::do_trade()
{
	std::vector<metternich::country *> trade_countries = this->get_countries();

	std::erase_if(trade_countries, [this](const country *country) {
		if (country->get_game_data()->is_under_anarchy()) {
			return true;
		}

		return false;
	});

	std::sort(trade_countries.begin(), trade_countries.end(), [&](const metternich::country *lhs, const metternich::country *rhs) {
		//give trade priority by opinion-weighted prestige
		const int lhs_prestige = lhs->get_game_data()->get_stored_commodity(defines::get()->get_prestige_commodity());
		const int rhs_prestige = rhs->get_game_data()->get_stored_commodity(defines::get()->get_prestige_commodity());

		if (lhs_prestige != rhs_prestige) {
			return lhs_prestige > rhs_prestige;
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	country_map<commodity_map<int>> country_luxury_demands;
	static constexpr int great_power_luxury_demand_divisor = 10;

	for (const country *country : trade_countries) {
		country_game_data *country_game_data = country->get_game_data();

		for (const auto &[commodity, demand] : country_game_data->get_commodity_demands()) {
			if (!country_game_data->can_trade_commodity(commodity)) {
				continue;
			}

			//increase demand if prices are lower than the base price, or the inverse if they are higher
			const centesimal_int effective_demand = demand * commodity->get_base_price() / game::get()->get_price(commodity);

			int effective_demand_int = effective_demand.to_int();
			if (country->is_great_power()) {
				effective_demand_int /= great_power_luxury_demand_divisor;
			}

			assert_throw(effective_demand_int >= 0);

			if (effective_demand_int == 0) {
				continue;
			}

			const int offer = country_game_data->get_offer(commodity);
			if (offer > 0) {
				const int sold_quantity = std::min(offer, effective_demand_int);

				if (sold_quantity <= 0) {
					continue;
				}

				country_game_data->do_sale(country, commodity, sold_quantity, false);

				effective_demand_int -= sold_quantity;
			}

			if (effective_demand_int > 0) {
				country_luxury_demands[country][commodity] = effective_demand_int;
			}
		}
	}

	for (const country *country : trade_countries) {
		country->get_game_data()->do_trade(country_luxury_demands);
	}

	//change commodity prices based on whether there were unfulfilled bids/offers
	commodity_map<int> remaining_demands;
	for (const country *country : trade_countries) {
		for (const auto &[commodity, bid] : country->get_game_data()->get_bids()) {
			remaining_demands[commodity] += bid;
		}

		for (const auto &[commodity, offer] : country->get_game_data()->get_offers()) {
			remaining_demands[commodity] -= offer;
		}
	}
	for (const auto &[country, luxury_demands] : country_luxury_demands) {
		for (const auto &[commodity, demand] : luxury_demands) {
			remaining_demands[commodity] += demand;
		}
	}

	for (const auto &[commodity, value] : remaining_demands) {
		//change the price according to the extra quantity bid/offered
		const int change = number::sqrt(std::abs(value)) * number::sign(value);

		if (change == 0) {
			continue;
		}

		this->change_price(commodity, change);
	}
}

QDate game::get_next_date() const
{
	return this->get_date().addMonths(defines::get()->get_months_per_turn(this->get_year()));
}

void game::increment_turn()
{
	const QDate old_date = this->get_date();
	this->date = this->get_next_date();
	assert_throw(this->get_date() != old_date);

	++this->turn;

	emit turn_changed();
}

std::string game::get_date_string() const
{
	if (defines::get()->get_months_per_turn(this->get_year()) % 12 != 0) {
		return std::format("{}, {}", date::get_month_season_string(this->get_date().month()), date::year_to_labeled_string(this->get_year()));
	} else {
		return date::year_to_labeled_string(this->get_year());
	}
}

QVariantList game::get_countries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_countries());
}

void game::add_country(country *country)
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
	country->get_game_data()->clear_leaders();

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
	std::vector<const metternich::country *> great_powers = game::get()->get_great_powers();

	if (great_powers.empty()) {
		return;
	}

	std::sort(great_powers.begin(), great_powers.end(), [](const metternich::country *lhs, const metternich::country *rhs) {
		if (lhs->get_game_data()->is_under_anarchy() != rhs->get_game_data()->is_under_anarchy()) {
			return rhs->get_game_data()->is_under_anarchy();
		}

		return lhs->get_game_data()->get_score() > rhs->get_game_data()->get_score();
	});

	int64_t average_score = 0;
	int highest_score = great_powers.at(0)->get_game_data()->get_score();
	
	for (size_t i = 0; i < great_powers.size(); ++i) {
		const country *great_power = great_powers.at(i);
		great_power->get_game_data()->set_score_rank(static_cast<int>(i));

		average_score += great_power->get_game_data()->get_score();
	}

	average_score /= great_powers.size();

	for (const country *great_power : great_powers) {
		const country_rank *best_rank = nullptr;
		for (const country_rank *rank : country_rank::get_all()) {
			if (best_rank != nullptr && best_rank->get_priority() >= rank->get_priority()) {
				continue;
			}

			const centesimal_int score(great_power->get_game_data()->get_score());
			const centesimal_int average_score_threshold = rank->get_average_score_threshold() * average_score;
			const centesimal_int relative_score_threshold = rank->get_relative_score_threshold() * highest_score;
			if (score >= average_score_threshold || score >= relative_score_threshold) {
				best_rank = rank;
			}
		}

		if (best_rank != nullptr) {
			great_power->get_game_data()->set_rank(best_rank);
		}
	}
}

int game::get_price(const commodity *commodity) const
{
	const auto find_iterator = this->prices.find(commodity);

	if (find_iterator != this->prices.end()) {
		return find_iterator->second;
	}

	return commodity->get_base_price();
}

void game::set_price(const commodity *commodity, const int value)
{
	if (value == this->get_price(commodity)) {
		return;
	}

	if (value < 1) {
		this->set_price(commodity, 1);
		return;
	}

	if (value == commodity->get_base_price()) {
		this->prices.erase(commodity);
	} else {
		this->prices[commodity] = value;
	}
}

QCoro::Task<void> game::create_diplomatic_map_image()
{
	std::vector<QCoro::Task<void>> tasks;

	if (map::get()->get_ocean_diplomatic_map_image().isNull()) {
		QCoro::Task<void> task = map::get()->create_ocean_diplomatic_map_image();
		tasks.push_back(std::move(task));
	}

	for (const country *country : this->get_countries()) {
		country_game_data *country_game_data = country->get_game_data();

		QCoro::Task<void> task = country_game_data->create_diplomatic_map_image();
		tasks.push_back(std::move(task));
	}

	for (QCoro::Task<void> &task : tasks) {
		co_await std::move(task);
	}
}

QCoro::Task<void> game::create_exploration_diplomatic_map_image()
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

	co_await QtConcurrent::run([this, final_scale_factor, &scaled_exploration_diplomatic_map_image]() {
		scaled_exploration_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(this->exploration_diplomatic_map_image, final_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
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

bool game::do_battle(army *attacking_army, army *defending_army)
{
	//this function returns true if the attackers won, or false otherwise

	const int attacker_score = attacking_army->get_score();
	const int defender_score = defending_army->get_score();

	if (attacker_score > defender_score) {
		//destroy the defender units
		for (military_unit *military_unit : defending_army->get_military_units()) {
			military_unit->disband(true);
		}

		return true;
	}

	if (attacker_score < defender_score) {
		//destroy the attacker units
		for (military_unit *military_unit : attacking_army->get_military_units()) {
			military_unit->disband(true);
		}
	}

	return false;
}

}
