#include "metternich.h"

#include "game/game.h"

#include "character/character.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "database/database.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_parser.h"
#include "database/gsml_property.h"
#include "database/preferences.h"
#include "domain/country.h"
#include "domain/country_ai.h"
#include "domain/country_economy.h"
#include "domain/country_game_data.h"
#include "domain/country_government.h"
#include "domain/country_history.h"
#include "domain/country_military.h"
#include "domain/country_rank.h"
#include "domain/country_technology.h"
#include "domain/country_tier.h"
#include "domain/country_turn_data.h"
#include "domain/cultural_group.h"
#include "domain/culture.h"
#include "domain/culture_history.h"
#include "domain/diplomacy_state.h"
#include "domain/government_type.h"
#include "domain/law.h"
#include "domain/office.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/character_event.h"
#include "game/country_event.h"
#include "game/game_rules.h"
#include "game/province_event.h"
#include "game/scenario.h"
#include "game/site_event.h"
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
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/effect/delayed_effect_instance.h"
#include "time/era.h"
#include "ui/portrait.h"
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
#include "unit/military_unit_stat.h"
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
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/path_util.h"
#include "util/size_util.h"
#include "util/string_conversion_util.h"
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
	const std::string &value = property.get_value();

	if (key == "date") {
		this->date = string::to_date(value);
	} else if (key == "turn") {
		this->turn = std::stoi(value);
	} else if (key == "player_character") {
		this->player_character = character::get(value);
	} else if (key == "player_country") {
		this->player_country = country::get(value);
	} else {
		throw std::runtime_error(std::format("Invalid game data property: \"{}\".", key));
	}
}

void game::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "rules") {
		auto rules = make_qunique<game_rules>();
		scope.process(rules.get());
		this->rules = std::move(rules);
	} else if (tag == "character_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const character>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "country_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const country>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "province_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const province>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "site_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const site>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "fired_events") {
		scope.for_each_child([&](const gsml_data &fired_events_data) {
			const std::string &event_type_string = fired_events_data.get_tag();
			const std::vector<std::string> &child_values = fired_events_data.get_values();

			for (const std::string &child_value : child_values) {
				const metternich::event *event = nullptr;
				if (event_type_string == "character") {
					event = character_event::get(child_value);
				} else if (event_type_string == "country") {
					event = country_event::get(child_value);
				} else if (event_type_string == "province") {
					event = province_event::get(child_value);
				} else if (event_type_string == "site") {
					event = site_event::get(child_value);
				} else {
					assert_throw(false);
				}

				this->fired_events.insert(event);
			}
		});
	} else if (tag == "countries") {
		scope.for_each_child([&](const gsml_data &country_data) {
			const country *country = country::get(country_data.get_tag());
			country_data.process(country->get_game_data());
		});
	} else if (tag == "characters") {
		scope.for_each_child([&](const gsml_data &character_data) {
			const character *character = character::get(character_data.get_tag());
			character_data.process(character->get_game_data());
		});
	} else if (tag == "generated_characters") {
		scope.for_each_child([&](const gsml_data &character_data) {
			auto generated_character = make_qunique<character>(character_data.get_tag());
			generated_character->moveToThread(QApplication::instance()->thread());
			character_data.process(generated_character.get());
		});
	} else {
		throw std::runtime_error(std::format("Invalid game data scope: \"{}\".", tag));
	}
}

gsml_data game::to_gsml_data() const
{
	gsml_data data;

	data.add_child("rules", this->get_rules()->to_gsml_data());

	data.add_property("date", date::to_string(this->get_date()));
	data.add_property("turn", std::to_string(this->turn));
	data.add_property("player_character", this->get_player_character()->get_identifier());
	data.add_property("player_country", this->get_player_country()->get_identifier());

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

	if (!this->fired_events.empty()) {
		gsml_data fired_events_data("fired_events");
		for (const metternich::event *fired_event : this->fired_events) {
			const std::string event_type_string(fired_event->get_event_type_string());
			if (!fired_events_data.has_child(event_type_string)) {
				gsml_data fired_events_type_data(event_type_string);
				fired_events_data.add_child(std::move(fired_events_type_data));
			}
			
			fired_events_data.get_child(event_type_string).add_value(fired_event->get_identifier());
		}
		data.add_child(std::move(fired_events_data));
	}

	gsml_data countries_data("countries");
	for (const country *country : this->get_countries()) {
		countries_data.add_child(country->get_game_data()->to_gsml_data());
	}
	data.add_child(std::move(countries_data));

	gsml_data characters_data("characters");
	for (const character *character : character::get_all()) {
		characters_data.add_child(character->get_game_data()->to_gsml_data());
	}
	data.add_child(std::move(characters_data));

	gsml_data generated_characters_data("generated_characters");
	for (const qunique_ptr<character> &character : this->get_generated_characters()) {
		generated_characters_data.add_child(character->to_gsml_data());
	}
	data.add_child(std::move(generated_characters_data));

	return data;
}

void game::save(const std::filesystem::path &filepath) const
{
	try {
		const gsml_data game_data = this->to_gsml_data();
		game_data.print_to_file(filepath);
	} catch (...) {
		exception::report(std::current_exception());
	}
}

void game::save(const QUrl &filepath) const
{
	this->save(path::from_qurl(filepath));
}

void game::load(const std::filesystem::path &filepath)
{
	try {
		if (!std::filesystem::exists(filepath)) {
			return;
		}

		gsml_parser parser;
		gsml_data data;

		try {
			data = parser.parse(filepath);
		} catch (...) {
			exception::report(std::current_exception());
			log::log_error(std::format("Failed to parse save file: {}", path::to_string(filepath)));
		}

		data.process(this);
	} catch (...) {
		exception::report(std::current_exception());
	}
}

void game::load(const QUrl &filepath)
{
	this->load(path::from_qurl(filepath));
}

QCoro::Task<void> game::setup_scenario_coro(metternich::scenario *scenario)
{
	try {
		const metternich::scenario *old_scenario = this->scenario;

		this->clear();
		this->scenario = scenario;

		this->date = game::normalize_date(scenario->get_start_date());

		database::get()->load_history(scenario->get_start_date(), scenario->get_timeline(), this->get_rules());

		if (old_scenario == nullptr || old_scenario->get_map_template() != scenario->get_map_template() || scenario->get_map_template()->is_randomly_generated()) {
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
	try {
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

			site->get_game_data()->check_settlement_type();
			site->get_game_data()->calculate_commodity_outputs();
		}

		for (const country *country : this->get_countries()) {
			country_game_data *country_game_data = country->get_game_data();
			country_government *country_government = country->get_government();

			for (population_unit *population_unit : country_game_data->get_population_units()) {
				population_unit->choose_ideology();
			}

			for (const office *office : office::get_all()) {
				country_government->check_office_holder(office, nullptr);
			}

			country_game_data->check_ideas();

			//setup journal entries, marking the ones for which the country already fulfills conditions as finished, but without doing the effects
			country_game_data->check_journal_entries(true, true);
		}

		this->set_running(true);
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("Failed to start game.");
		QApplication::exit(EXIT_FAILURE);
	}
}

void game::stop()
{
	try {
		if (!this->is_running()) {
			//already stopped
			return;
		}

		this->set_running(false);
		this->clear();
		map::get()->clear();
		this->set_player_character(nullptr);
		this->set_player_country(nullptr);
	} catch (...) {
		exception::report(std::current_exception());
	}
}

void game::clear()
{
	try {
		this->rules = preferences::get()->get_game_rules()->duplicate();

		this->clear_delayed_effects();
		this->fired_events.clear();

		this->reset_game_data();

		map::get()->clear_tile_game_data();

		this->scenario = nullptr;
		this->countries.clear();
		this->wonder_countries.clear();
		this->prices.clear();

		this->date = game::normalize_date(defines::get()->get_default_start_date());
		this->turn = 1;

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

	this->generated_characters.clear();
}

void game::apply_history(const metternich::scenario *scenario)
{
	try {
		for (const province *province : map::get()->get_provinces()) {
			try {
				const province_history *province_history = province->get_history();
				province_game_data *province_game_data = province->get_game_data();

				if (province->is_water_zone()) {
					continue;
				}

				province_game_data->set_culture(province_history->get_main_culture());
				province_game_data->set_religion(province_history->get_religion());

				const country *owner = province_history->get_owner();
				province_game_data->set_owner(owner);

				if (owner == nullptr) {
					log::log_error(std::format("Province \"{}\" has no owner for scenario \"{}\".", province->get_identifier(), scenario->get_identifier()));
				}
			} catch (...) {
				std::throw_with_nested(std::runtime_error(std::format("Failed to apply history for province \"{}\".", province->get_identifier())));
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

		for (const culture *culture : culture::get_all()) {
			const culture_history *culture_history = culture->get_history();

			for (const country *country : this->get_countries()) {
				if (country->get_culture() != culture) {
					continue;
				}

				culture_history->apply_to_country(country);
			}
		}

		for (const country *country : this->get_countries()) {
			const country_history *country_history = country->get_history();
			country_game_data *country_game_data = country->get_game_data();
			country_economy *country_economy = country->get_economy();
			country_government *country_government = country->get_government();
			country_technology *country_technology = country->get_technology();

			if (country_history->get_tier() != country_tier::none) {
				country->get_game_data()->set_tier(country_history->get_tier());
			}

			if (country_history->get_religion() != nullptr) {
				country_game_data->set_religion(country_history->get_religion());
			}

			const subject_type *subject_type = country_history->get_subject_type();
			if (subject_type != nullptr) {
				//disable overlordship for now
				//country_game_data->set_subject_type(subject_type);
			}

			if (country_history->get_government_type() != nullptr) {
				country_government->set_government_type(country_history->get_government_type());

				if (country_history->get_government_type()->get_required_technology() != nullptr) {
					country_technology->add_technology_with_prerequisites(country_history->get_government_type()->get_required_technology());
				}
			} else if (country->get_default_government_type() != nullptr) {
				country_government->set_government_type(country->get_default_government_type());

				if (country->get_default_government_type()->get_required_technology() != nullptr) {
					country_technology->add_technology_with_prerequisites(country->get_default_government_type()->get_required_technology());
				}
			}

			for (const auto &[office, office_holder] : country_history->get_office_holders()) {
				assert_throw(scenario->get_start_date() >= office_holder->get_start_date());
				if (office_holder->get_death_date().isValid() && scenario->get_start_date() >= office_holder->get_death_date()) {
					continue;
				}

				character_game_data *office_holder_game_data = office_holder->get_game_data();

				if (office_holder_game_data->get_country() != nullptr && office_holder_game_data->get_country() != country) {
					throw std::runtime_error(std::format("Cannot set \"{}\" as an office holder for \"{}\", as they are already assigned to another country.", office_holder->get_identifier(), country->get_identifier()));
				}

				country_government->set_office_holder(office, office_holder);
			}

			for (const technology *technology : country_history->get_technologies()) {
				country_technology->add_technology_with_prerequisites(technology);
			}

			for (const auto &[law_group, law] : country_history->get_laws()) {
				country_government->set_law(law_group, law);

				if (law->get_required_technology() != nullptr) {
					country_technology->add_technology_with_prerequisites(law->get_required_technology());
				}
			}

			country_economy->set_wealth(country_history->get_wealth());

			for (const auto &[other_country, diplomacy_state] : country_history->get_diplomacy_states()) {
				if (!other_country->get_game_data()->is_alive()) {
					continue;
				}

				if (is_vassalage_diplomacy_state(diplomacy_state) || is_overlordship_diplomacy_state(diplomacy_state)) {
					//disable overlordship diplomacy states for now
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
			country_government *country_government = country->get_government();

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

			country_government->check_government_type();
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
								tile->get_owner()->get_technology()->add_technology_with_prerequisites(route_pathway->get_river_crossing_required_technology());
							}
						}
					}

					tile->calculate_pathway_frames();

					//add prerequisites for the tile's pathway to its owner's researched technologies
					if (tile->get_owner() != nullptr) {
						if (route_pathway->get_required_technology() != nullptr) {
							tile->get_owner()->get_technology()->add_technology_with_prerequisites(route_pathway->get_required_technology());
						}

						const technology *terrain_required_technology = route_pathway->get_terrain_required_technology(tile->get_terrain());
						if (terrain_required_technology != nullptr) {
							tile->get_owner()->get_technology()->add_technology_with_prerequisites(terrain_required_technology);
						}
					}
				}
			}
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
				country->get_economy()->set_stored_commodity(commodity, quantity);
			}
		}

		for (const character *character : character::get_all()) {
			character_game_data *character_game_data = character->get_game_data();
			character_game_data->apply_history(scenario->get_start_date());
		}

		for (const historical_civilian_unit *historical_civilian_unit : historical_civilian_unit::get_all()) {
			try {
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
				country_technology *owner_technology = owner->get_technology();

				assert_throw(owner_game_data->is_alive());

				if (owner_game_data->is_under_anarchy()) {
					continue;
				}

				const civilian_unit_type *type = historical_civilian_unit->get_type();
				assert_throw(type != nullptr);

				if (type->get_required_technology() != nullptr) {
					owner_technology->add_technology_with_prerequisites(type->get_required_technology());
				}

				const phenotype *phenotype = historical_civilian_unit->get_phenotype();
				const bool created = owner_game_data->create_civilian_unit(historical_civilian_unit->get_type(), site, phenotype);
				assert_throw(created);
			} catch (...) {
				std::throw_with_nested(std::runtime_error(std::format("Failed to apply historical civilian unit \"{}\".", historical_civilian_unit->get_identifier())));
			}
		}

		for (const historical_military_unit *historical_military_unit : historical_military_unit::get_all()) {
			try {
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
				if (country != nullptr && !country->get_game_data()->is_alive()) {
					continue;
				}

				if (country == nullptr) {
					country = province->get_game_data()->get_owner();
				}

				assert_throw(country != nullptr);

				country_game_data *country_game_data = country->get_game_data();
				country_military *country_military = country->get_military();
				country_technology *country_technology = country->get_technology();

				assert_throw(country_game_data->is_alive());

				const military_unit_type *type = historical_military_unit->get_type();
				assert_throw(type != nullptr);

				if (type->get_required_technology() != nullptr) {
					country_technology->add_technology_with_prerequisites(type->get_required_technology());
				}

				const phenotype *phenotype = historical_military_unit->get_phenotype();

				for (int i = 0; i < historical_military_unit->get_quantity(); ++i) {
					const bool created = country_military->create_military_unit(type, province, phenotype, historical_military_unit_history->get_promotions());
					assert_throw(created);
				}
			} catch (...) {
				std::throw_with_nested(std::runtime_error(std::format("Failed to apply historical military unit \"{}\".", historical_military_unit->get_identifier())));
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
			country_technology *country_technology = country->get_technology();

			if (!country_game_data->is_alive()) {
				continue;
			}

			if (country_game_data->is_under_anarchy()) {
				continue;
			}

			const transporter_type *type = historical_transporter->get_type();
			assert_throw(type != nullptr);

			if (type->get_required_technology() != nullptr) {
				country_technology->add_technology_with_prerequisites(type->get_required_technology());
			}

			const phenotype *phenotype = historical_transporter->get_phenotype();

			for (int i = 0; i < historical_transporter->get_quantity(); ++i) {
				const bool created = country_game_data->create_transporter(type, phenotype);
				assert_throw(created);
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to apply history for scenario \"{}\".", scenario->get_identifier())));
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

		if (!provincial_capital->get_map_data()->is_on_map()) {
			continue;
		}

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

	//set the capitals here, so that building requirements that require a capital can be fulfilled
	for (const country *country : this->get_countries()) {
		country->get_game_data()->set_capital(nullptr);
		country->get_game_data()->choose_capital();
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

			if (site->get_map_data()->get_resource() != nullptr && site_history->is_developed() && (!site_improvements.contains(improvement_slot::resource) || site_improvements.find(improvement_slot::resource)->second->get_level() < site_history->get_development_level())) {
				//if the site is marked as developed, but has no specific improvement set for it, or has an improvement with a level below its development level, pick an appropriate improvement for its resource
				for (const improvement *improvement : site->get_map_data()->get_resource()->get_improvements()) {
					if (improvement->get_level() != site_history->get_development_level()) {
						continue;
					}

					site_improvements[improvement_slot::resource] = improvement;
					break;
				}
			}

			for (const auto &[improvement_slot, improvement] : site_improvements) {
				if (improvement != nullptr) {
					if (!improvement->get_resources().empty()) {
						assert_throw(site->get_map_data()->get_type() == site_type::resource || site->get_map_data()->get_type() == site_type::celestial_body || site->is_settlement());

						if (tile->get_resource() == nullptr) {
							throw std::runtime_error(std::format("Failed to set resource improvement for tile for site \"{}\", as it has no resource.", site->get_identifier()));
						}

						if (!vector::contains(improvement->get_resources(), tile->get_resource())) {
							throw std::runtime_error(std::format("Failed to set resource improvement for tile for site \"{}\", as its resource is different than that of the improvement.", site->get_identifier()));
						}

						map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
					}

					site_game_data->set_improvement(improvement->get_slot(), improvement);

					//add prerequisites for the tile's improvement to its owner's researched technologies
					if (improvement->get_required_technology() != nullptr && tile->get_owner() != nullptr) {
						tile->get_owner()->get_technology()->add_technology_with_prerequisites(improvement->get_required_technology());
					}
				}
			}

			if (site_history->is_resource_discovered()) {
				assert_throw(site->get_map_data()->get_type() == site_type::resource || site->get_map_data()->get_type() == site_type::celestial_body || site->is_settlement());
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

	if (settlement == nullptr || !settlement->get_map_data()->is_on_map()) {
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
	country_technology *owner_technology = owner ? owner->get_technology() : nullptr;

	for (auto [building_slot_type, building] : site_history->get_buildings()) {
		building_slot *building_slot = nullptr;
		if (building->is_provincial()) {
			building_slot = settlement_game_data->get_building_slot(building_slot_type);
		} else {
			continue;
		}

		assert_throw(building_slot != nullptr);

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
			continue;
		}

		if (slot_building == nullptr || slot_building->get_level() < building->get_level()) {
			settlement_game_data->set_slot_building(building_slot_type, building);
		}

		if (building->get_required_technology() != nullptr && owner_game_data != nullptr) {
			owner_technology->add_technology_with_prerequisites(building->get_required_technology());
		}
	}

	for (auto [building_slot_type, wonder] : site_history->get_wonders()) {
		if (!wonder->is_enabled()) {
			continue;
		}

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
			owner_technology->add_technology_with_prerequisites(wonder->get_required_technology());
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

		if (province->get_history()->get_population() > 0) {
			const int province_level = defines::get()->get_province_level_for_population(province->get_history()->get_population());
			static constexpr int max_province_level = 2;
			province_game_data->set_level(std::min(std::max(province_level, 1), max_province_level));
		}

		if (province_game_data->get_settlement_count() == 0) {
			continue;
		}

		province->get_history()->distribute_population();

		population_group_map<int> remaining_province_populations;

		for (const site *site : province_game_data->get_sites()) {
			site_game_data *site_game_data = site->get_game_data();

			if (!site_game_data->can_have_population()) {
				continue;
			}

			site_history *site_history = site->get_history();

			site_history->initialize_population();

			for (const auto &[group_key, population] : site_history->get_population_groups()) {
				if (population <= 0) {
					continue;
				}

				const int64_t remaining_population = this->apply_historical_population_group_to_site(group_key, population, site);

				if (remaining_population != 0 && site_game_data->get_owner() != nullptr) {
					//add the remaining population to remaining population data for the province
					remaining_province_populations[group_key] += remaining_population;
				}
			}
		}

		const site *provincial_capital = province->get_provincial_capital();
		assert_throw(provincial_capital != nullptr);

		for (const auto &[group_key, population] : remaining_province_populations) {
			if (population <= 0) {
				continue;
			}

			const int64_t remaining_population = this->apply_historical_population_group_to_site(group_key, population, provincial_capital);

			if (remaining_population != 0 && provincial_capital->get_game_data()->get_owner() != nullptr) {
				//add the remaining population to remaining population data for the owner
				country_populations[provincial_capital->get_game_data()->get_owner()][group_key] += remaining_population;
			}
		}

		if (provincial_capital->get_game_data()->get_population_units().empty()) {
			//ensure provincial capital settlements have at least one population unit
			this->apply_historical_population_units_to_site(population_group_key(), 1, provincial_capital);
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

			const int64_t remaining_population = this->apply_historical_population_group_to_site(group_key, population, capital);

			//add the remaining population to broader groups
			if (remaining_population > 0 && !group_key.is_empty()) {
				population_group_key group_key_copy = group_key;

				if (group_key.type != nullptr && !capital->get_game_data()->can_have_population_type(group_key.type)) {
					group_key_copy.type = nullptr;
				} else if (group_key.phenotype != nullptr) {
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

int64_t game::apply_historical_population_group_to_site(const population_group_key &group_key, const int population, const site *site)
{
	if (population <= 0) {
		return 0;
	}

	site_game_data *site_game_data = site->get_game_data();

	if (!site_game_data->is_built()) {
		return population;
	}

	if (group_key.type != nullptr && !site_game_data->can_have_population_type(group_key.type)) {
		return population;
	}

	log_trace(std::format("Applying historical population group of type \"{}\", culture \"{}\", religion \"{}\" and size {} for settlement \"{}\".", group_key.type ? group_key.type->get_identifier() : "none", group_key.culture ? group_key.culture->get_identifier() : "none", group_key.religion ? group_key.religion->get_identifier() : "none", population, site->get_identifier()));

	const country *country = site_game_data->get_owner();

	if (country == nullptr) {
		return 0;
	}

	int population_unit_count = population / defines::get()->get_population_per_unit();

	this->apply_historical_population_units_to_site(group_key, population_unit_count, site);

	int64_t remaining_population = population - (population_unit_count * defines::get()->get_population_per_unit());
	remaining_population = std::max<int64_t>(0, remaining_population);

	log_trace(std::format("Remaining population: {}.", remaining_population));

	return remaining_population;
}

void game::apply_historical_population_units_to_site(const population_group_key &group_key, int population_unit_count, const site *site)
{
	if (population_unit_count <= 0) {
		return;
	}

	site_game_data *site_game_data = site->get_game_data();

	if (!site_game_data->is_built()) {
		return;
	}

	const population_type *population_type = group_key.type;

	if (population_type != nullptr && !site_game_data->can_have_population_type(population_type)) {
		return;
	}

	site_history *site_history = site->get_history();

	const province *province = site->get_game_data()->get_province();
	province_history *province_history = province->get_history();

	const country *country = site_game_data->get_owner();

	if (country == nullptr) {
		return;
	}

	const culture *site_culture = site_history->get_culture();

	const culture *culture = group_key.culture;
	culture_map<int64_t> culture_weights;
	std::vector<const metternich::culture *> weighted_cultures;
	if (culture == nullptr) {
		if (site_culture != nullptr) {
			culture = site_culture;
		} else if (!province_history->get_culture_weights().empty()) {
			culture_weights = province_history->get_culture_weights();
			if (culture_weights.size() == 1) {
				culture = culture_weights.begin()->first;
			} else {
				weighted_cultures = archimedes::map::to_weighted_vector(culture_weights);
			}
		} else {
			log::log_error(std::format("Province \"{}\" has no culture weights.", province->get_identifier()));
			return;
		}
	}
	assert_throw(culture != nullptr || !weighted_cultures.empty());

	const religion *site_religion = site_history->get_religion();
	const religion *province_religion = province_history->get_religion();

	const religion *religion = group_key.religion;
	if (religion == nullptr) {
		if (site_religion != nullptr) {
			religion = site_religion;
		} else if (province_religion != nullptr) {
			religion = province_religion;
		} else {
			log::log_error(std::format("Province \"{}\" has no religion.", province->get_identifier()));
			return;
		}
	}
	assert_throw(religion != nullptr);

	const phenotype *phenotype = group_key.phenotype;
	culture_map<std::vector<const metternich::phenotype *>> culture_weighted_phenotypes;
	if (phenotype == nullptr) {
		if (culture != nullptr) {
			std::vector<const metternich::phenotype *> weighted_phenotypes;

			if (!province_history->get_phenotype_weights().empty()) {
				weighted_phenotypes = province_history->get_weighted_phenotypes_for_culture(culture);
			}

			if (weighted_phenotypes.empty()) {
				weighted_phenotypes = culture->get_weighted_phenotypes();
			}

			assert_throw(!weighted_phenotypes.empty());
			culture_weighted_phenotypes[culture] = std::move(weighted_phenotypes);
		} else {
			for (const auto &[potential_culture, weight] : culture_weights) {
				std::vector<const metternich::phenotype *> weighted_phenotypes;

				if (!province_history->get_phenotype_weights().empty()) {
					weighted_phenotypes = province_history->get_weighted_phenotypes_for_culture(potential_culture);
				}

				if (weighted_phenotypes.empty()) {
					weighted_phenotypes = potential_culture->get_weighted_phenotypes();
				}

				assert_throw(!weighted_phenotypes.empty());
				culture_weighted_phenotypes[potential_culture] = std::move(weighted_phenotypes);
			}
		}

		assert_throw(!culture_weighted_phenotypes.empty());
	}

	if (population_type == nullptr) {
		const population_class *literate_population_class = site->get_game_data()->get_default_literate_population_class();
		if (literate_population_class != nullptr) {
			centesimal_int literacy_rate = site_history->get_literacy_rate();
			if (literacy_rate == 0) {
				literacy_rate = province_history->get_literacy_rate();
			}
			if (literacy_rate == 0) {
				literacy_rate = country->get_history()->get_literacy_rate();
			}

			if (literacy_rate != 0) {
				const int literate_population_unit_count = (population_unit_count * literacy_rate / 100).to_int();

				for (int i = 0; i < literate_population_unit_count; ++i) {
					const metternich::culture *population_unit_culture = culture;
					if (population_unit_culture == nullptr && !weighted_cultures.empty()) {
						population_unit_culture = vector::get_random(weighted_cultures);
					}

					const metternich::population_type *unit_population_type = population_unit_culture->get_population_class_type(literate_population_class);

					if (!unit_population_type->is_enabled()) {
						continue;
					}

					const metternich::phenotype *population_unit_phenotype = phenotype;
					if (population_unit_phenotype == nullptr && !culture_weighted_phenotypes[population_unit_culture].empty()) {
						population_unit_phenotype = vector::get_random(culture_weighted_phenotypes[population_unit_culture]);
					}

					site_game_data->create_population_unit(unit_population_type, population_unit_culture, religion, population_unit_phenotype);

					--population_unit_count;
				}
			}
		}
	}

	const population_class *population_class = site->get_game_data()->get_default_population_class();

	for (int i = 0; i < population_unit_count; ++i) {
		const metternich::culture *population_unit_culture = culture;
		if (population_unit_culture == nullptr && !weighted_cultures.empty()) {
			population_unit_culture = vector::get_random(weighted_cultures);
		}

		const metternich::population_type *unit_population_type = population_type;
		if (population_type == nullptr) {
			unit_population_type = population_unit_culture->get_population_class_type(population_class);
		}
		assert_throw(unit_population_type != nullptr);

		const metternich::phenotype *population_unit_phenotype = phenotype;
		if (population_unit_phenotype == nullptr && !culture_weighted_phenotypes[population_unit_culture].empty()) {
			population_unit_phenotype = vector::get_random(culture_weighted_phenotypes[population_unit_culture]);
		}

		site_game_data->create_population_unit(unit_population_type, population_unit_culture, religion, population_unit_phenotype);
	}
}

QCoro::Task<void> game::on_setup_finished()
{
	co_await this->create_diplomatic_map_image();

	for (const province *province : map::get()->get_provinces()) {
		co_await province->get_game_data()->create_map_image();
	}

	emit countries_changed();

	for (const country *country : this->get_countries()) {
		country_game_data *country_game_data = country->get_game_data();
		country_economy *country_economy = country->get_economy();
		country_government *country_government = country->get_government();

		country_government->check_government_type();
		country_government->check_laws();

		for (const office *office : office::get_all()) {
			country_government->check_office_holder(office, nullptr);
		}

		country_game_data->check_ideas();

		for (const QPoint &border_tile_pos : country_game_data->get_border_tiles()) {
			map::get()->calculate_tile_country_border_directions(border_tile_pos);
		}

		for (const province *province : country_game_data->get_provinces()) {
			province->get_game_data()->check_governor();

			for (const site *site : province->get_game_data()->get_sites()) {
				for (const improvement *improvement : improvement::get_all()) {
					if (improvement->get_free_on_start_conditions() == nullptr) {
						continue;
					}

					if (!improvement->get_free_on_start_conditions()->check(site)) {
						continue;
					}

					site->get_game_data()->check_free_improvement(improvement);
				}

				if (site->get_map_data()->get_type() == site_type::resource || site->get_map_data()->get_type() == site_type::celestial_body) {
					site->get_game_data()->check_landholder();
				}
			}

			//build free on start buildings
			for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
				if (!settlement->get_game_data()->is_built()) {
					continue;
				}

				for (const building_type *building : building_type::get_all()) {
					if (!building->is_provincial()) {
						continue;
					}

					if (building->get_free_on_start_conditions() == nullptr) {
						continue;
					}

					if (!building->get_free_on_start_conditions()->check(settlement, read_only_context(settlement))) {
						continue;
					}

					settlement->get_game_data()->check_free_building(building);
				}
			}
		}

		//decrease population if there's too much for the starting food output
		while ((country_economy->get_food_output() - country_game_data->get_net_food_consumption()) < 0) {
			country_game_data->decrease_population(false);
		}

		emit country->game_data_changed();
	}

	this->calculate_country_ranks();

	for (const character *character : character::get_all()) {
		character->get_game_data()->on_setup_finished();

		emit character->game_data_changed();
	}

	emit setup_finished();
}

QCoro::Task<void> game::do_turn_coro()
{
	try {
		this->process_delayed_effects();

		country_map<commodity_map<int>> old_bids;
		country_map<commodity_map<int>> old_offers;

		for (country *country : this->get_countries()) {
			country->reset_turn_data();

			country->get_economy()->calculate_commodity_needs();

			if (country->get_game_data()->is_ai()) {
				country->get_ai()->do_turn();
			}

			old_bids[country] = country->get_economy()->get_bids();
			old_offers[country] = country->get_economy()->get_offers();
		}

		this->do_trade();

		for (const country *country : this->get_countries()) {
			country->get_game_data()->do_turn();
		}

		for (const country *country : this->get_countries()) {
			//do country events after processing the turn for each country, so that e.g. events won't refer to a scope which no longer exists by the time the player gets to choose an option
			country->get_game_data()->do_events();

			//restore old bids and offers, if possible
			for (const auto &[commodity, bid] : old_bids[country]) {
				country->get_economy()->set_bid(commodity, bid);
			}

			for (const auto &[commodity, offer] : old_offers[country]) {
				country->get_economy()->set_offer(commodity, offer);
			}
		}

		for (const country *country : this->get_countries()) {
			if (country->get_turn_data()->is_diplomatic_map_dirty()) {
				co_await country->get_game_data()->create_diplomatic_map_image();

				//FIXME: add province turn data, and allow setting a "province map dirty" property for it to true, for recreating the province map image
				for (const province *province : country->get_game_data()->get_provinces()) {
					co_await province->get_game_data()->create_map_image();
				}
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

		this->calculate_country_ranks();

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
		if (defines::get()->get_prestige_commodity()->is_enabled()) {
			//give trade priority by prestige
			const int lhs_prestige = lhs->get_economy()->get_stored_commodity(defines::get()->get_prestige_commodity());
			const int rhs_prestige = rhs->get_economy()->get_stored_commodity(defines::get()->get_prestige_commodity());

			if (lhs_prestige != rhs_prestige) {
				return lhs_prestige > rhs_prestige;
			}
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	country_map<commodity_map<int>> country_luxury_demands;

	for (const country *country : trade_countries) {
		country_economy *country_economy = country->get_economy();

		for (const auto &[commodity, demand] : country_economy->get_commodity_demands()) {
			if (!country_economy->can_trade_commodity(commodity)) {
				continue;
			}

			//increase demand if prices are lower than the base price, or the inverse if they are higher
			decimillesimal_int effective_demand = demand * commodity->get_base_price() / game::get()->get_price(commodity);

			int effective_demand_int = effective_demand.to_int();

			assert_throw(effective_demand_int >= 0);

			if (effective_demand_int == 0) {
				continue;
			}

			const int offer = country_economy->get_offer(commodity);
			if (offer > 0) {
				const int sold_quantity = std::min(offer, effective_demand_int);

				if (sold_quantity <= 0) {
					continue;
				}

				country_economy->do_sale(country, commodity, sold_quantity, false);

				effective_demand_int -= sold_quantity;
			}

			if (effective_demand_int > 0) {
				country_luxury_demands[country][commodity] = effective_demand_int;
			}
		}
	}

	for (const country *country : trade_countries) {
		country->get_economy()->do_trade(country_luxury_demands);
	}

	//change commodity prices based on whether there were unfulfilled bids/offers
	commodity_map<int> remaining_demands;
	for (const country *country : trade_countries) {
		for (const auto &[commodity, bid] : country->get_economy()->get_bids()) {
			remaining_demands[commodity] += bid;
		}

		for (const auto &[commodity, offer] : country->get_economy()->get_offers()) {
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

int game::get_current_months_per_turn() const
{
	return defines::get()->get_months_per_turn(this->get_year());
}

QDate game::get_next_date() const
{
	return this->get_date().addMonths(this->get_current_months_per_turn());
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
	if (this->get_current_months_per_turn() % 12 != 0) {
		if (this->get_current_months_per_turn() == 3) {
			return std::format("{}, {}", date::get_month_season_string(this->get_date().month()), date::year_to_labeled_string(this->get_year()));
		} else {
			return std::format("{}, {}", date::get_month_name(this->get_date().month()), date::year_to_labeled_string(this->get_year()));
		}
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

	if (this->is_running()) {
		emit countries_changed();
	}
}

void game::remove_country(country *country)
{
	std::erase(this->countries, country);

	country->get_military()->clear_leaders();

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

void game::calculate_country_ranks()
{
	std::vector<metternich::country *> countries = game::get()->get_countries();

	if (countries.empty()) {
		return;
	}

	std::sort(countries.begin(), countries.end(), [](const metternich::country *lhs, const metternich::country *rhs) {
		if (lhs->get_game_data()->is_under_anarchy() != rhs->get_game_data()->is_under_anarchy()) {
			return rhs->get_game_data()->is_under_anarchy();
		}

		return lhs->get_game_data()->get_score() > rhs->get_game_data()->get_score();
	});

	int64_t average_score = 0;
	int highest_score = countries.at(0)->get_game_data()->get_score();
	
	for (size_t i = 0; i < countries.size(); ++i) {
		const country *country = countries.at(i);
		country->get_game_data()->set_score_rank(static_cast<int>(i));

		average_score += country->get_game_data()->get_score();
	}

	average_score /= countries.size();

	for (const country *country : countries) {
		const country_rank *best_rank = nullptr;
		for (const country_rank *rank : country_rank::get_all()) {
			if (best_rank != nullptr && best_rank->get_priority() >= rank->get_priority()) {
				continue;
			}

			const centesimal_int score(country->get_game_data()->get_score());
			const centesimal_int average_score_threshold = rank->get_average_score_threshold() * average_score;
			const centesimal_int relative_score_threshold = rank->get_relative_score_threshold() * highest_score;
			if (score >= average_score_threshold || score >= relative_score_threshold) {
				best_rank = rank;
			}
		}

		if (best_rank != nullptr) {
			country->get_game_data()->set_rank(best_rank);
		}
	}
}

void game::set_player_country(const country *country)
{
	if (country == this->get_player_country()) {
		return;
	}

	this->player_country = country;

	if (this->is_running()) {
		emit player_country_changed();
	}

	this->set_player_character(country ? country->get_government()->get_ruler() : nullptr);
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

	co_await QtConcurrent::run([this, tile_pixel_size, &scaled_exploration_diplomatic_map_image]() {
		scaled_exploration_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(this->exploration_diplomatic_map_image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->exploration_diplomatic_map_image = std::move(scaled_exploration_diplomatic_map_image);

	//make semi-transparent pixels non-transparent
	for (int x = 0; x < this->exploration_diplomatic_map_image.width(); ++x) {
		for (int y = 0; y < this->exploration_diplomatic_map_image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			QColor pixel_color = this->exploration_diplomatic_map_image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0 || pixel_color.alpha() == 255) {
				continue;
			}

			pixel_color.setAlpha(255);
			this->exploration_diplomatic_map_image.setPixelColor(pixel_pos, pixel_color);
		}
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	co_await QtConcurrent::run([this, scale_factor, &scaled_exploration_diplomatic_map_image]() {
		scaled_exploration_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(this->exploration_diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->exploration_diplomatic_map_image = std::move(scaled_exploration_diplomatic_map_image);

	assert_throw(this->exploration_diplomatic_map_image.size() == map::get()->get_diplomatic_map_image_size() * preferences::get()->get_scale_factor());
	assert_throw(this->exploration_diplomatic_map_image.size
	() == map::get()->get_ocean_diplomatic_map_image().size());
}

void game::add_generated_character(qunique_ptr<character> &&character)
{
	this->generated_characters.push_back(std::move(character));
}

void game::remove_generated_character(character *character)
{
	vector::remove(this->generated_characters, character);
}

void game::process_delayed_effects()
{
	this->process_delayed_effects(this->character_delayed_effects);
	this->process_delayed_effects(this->country_delayed_effects);
	this->process_delayed_effects(this->province_delayed_effects);
	this->process_delayed_effects(this->site_delayed_effects);
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

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<const site>> &&delayed_effect)
{
	this->site_delayed_effects.push_back(std::move(delayed_effect));
}

void game::clear_delayed_effects()
{
	this->character_delayed_effects.clear();
	this->country_delayed_effects.clear();
	this->province_delayed_effects.clear();
	this->site_delayed_effects.clear();
}

bool game::has_fired_event(const metternich::event *event) const
{
	return this->fired_events.contains(event);
}

void game::add_fired_event(const metternich::event *event)
{
	this->fired_events.insert(event);
}

bool game::do_battle(army *attacking_army, army *defending_army)
{
	//this function returns true if the attackers won, or false otherwise

	static constexpr dice battle_dice(1, 6);

	military_unit_type_map<int> lost_unit_count;
	//keep track of the player's starting unit count, so that we can later calculate their lost unit count
	if (attacking_army->get_country() == this->get_player_country()) {
		for (const military_unit *military_unit : attacking_army->get_military_units()) {
			lost_unit_count[military_unit->get_type()]++;
		}
	} else if (defending_army->get_country() == this->get_player_country()) {
		for (const military_unit *military_unit : defending_army->get_military_units()) {
			lost_unit_count[military_unit->get_type()]++;
		}
	}

	const province *battle_province = attacking_army->get_target_province();
	if (battle_province == nullptr && !defending_army->get_military_units().empty()) {
		battle_province = defending_army->get_military_units().at(0)->get_province();
	}
	if (battle_province == nullptr && !attacking_army->get_military_units().empty()) {
		//this could happen if the army is garrisoned in a province, and is "attacking" something by event
		battle_province = attacking_army->get_military_units().at(0)->get_province();
	}

	while (!attacking_army->get_military_units().empty() && !defending_army->get_military_units().empty()) {
		int attacker_hits = 0;
		for (const military_unit *attacking_unit : attacking_army->get_military_units()) {
			const int attack = std::max(std::max(attacking_unit->get_stat(military_unit_stat::melee).to_int(), attacking_unit->get_stat(military_unit_stat::charge).to_int()), attacking_unit->get_stat(military_unit_stat::missile).to_int());

			const int result = random::get()->roll_dice(battle_dice);
			if (result <= attack) {
				++attacker_hits;
			}
		}

		int defender_hits = 0;
		for (const military_unit *defending_unit : defending_army->get_military_units()) {
			const int defense = std::max(defending_unit->get_stat(military_unit_stat::defense).to_int(), defending_unit->get_stat(military_unit_stat::missile).to_int());

			const int result = random::get()->roll_dice(battle_dice);
			if (result <= defense) {
				++defender_hits;
			}
		}
		
		while (attacker_hits > 0 && !defending_army->get_military_units().empty()) {
			const std::vector<military_unit *> defending_units = defending_army->get_military_units();
			for (military_unit *defending_unit : defending_units) {
				defending_unit->change_hit_points(-1);
				--attacker_hits;
				if (attacker_hits == 0) {
					break;
				}
			}
		}
		
		while (defender_hits > 0 && !attacking_army->get_military_units().empty()) {
			const std::vector<military_unit *> attacking_units = attacking_army->get_military_units();
			for (military_unit *attacking_unit : attacking_units) {
				attacking_unit->change_hit_points(-1);
				--defender_hits;
				if (defender_hits == 0) {
					break;
				}
			}
		}
	}

	//restore hit points of the surviving units
	for (military_unit *attacking_unit : attacking_army->get_military_units()) {
		attacking_unit->set_hit_points(attacking_unit->get_max_hit_points());
	}
	for (military_unit *defending_unit : defending_army->get_military_units()) {
		defending_unit->set_hit_points(defending_unit->get_max_hit_points());
	}

	assert_throw(attacking_army->get_military_units().empty() || defending_army->get_military_units().empty());

	const bool attack_success = defending_army->get_military_units().empty() && !attacking_army->get_military_units().empty();

	//display a notification for the player about the battle
	if (attacking_army->get_country() == this->get_player_country() || defending_army->get_country() == this->get_player_country()) {
		const bool is_attacker = attacking_army->get_country() == this->get_player_country();
		const bool victory = (is_attacker == attack_success);
		const portrait *war_minister_portrait = this->get_player_country()->get_government()->get_war_minister_portrait();

		std::string lost_units_str;
		const std::vector<military_unit *> &remaining_military_units = is_attacker ? attacking_army->get_military_units() : defending_army->get_military_units();
		//remove the remaining military units from the lost unit count, so that we get the actual count of lost units
		for (const military_unit *military_unit : remaining_military_units) {
			lost_unit_count[military_unit->get_type()]--;
		}
		for (const auto &[military_unit_type, lost_count] : lost_unit_count) {
			if (lost_count == 0) {
				continue;
			}

			if (lost_units_str.empty()) {
				lost_units_str += "\n\nLost Units:";
			}

			lost_units_str += std::format("\n{} {}", lost_count, military_unit_type->get_name());
		}

		engine_interface::get()->add_notification(victory ? "Victory!" : "Defeat!", war_minister_portrait, std::format("We have {} a battle{}!{}", victory ? "won" : "lost", battle_province != nullptr ? std::format(" in {}", battle_province->get_game_data()->get_current_cultural_name()) : "", lost_units_str));
	}

	return attack_success;
}

bool game::do_combat(const std::vector<const character *> &attackers, const std::vector<const character *> &defenders)
{
	//this function returns true if the attackers won, or false otherwise

	static constexpr dice initiative_dice(1, 10);

	std::vector<const character *> surviving_attackers = attackers;
	std::vector<const character *> surviving_defenders = defenders;

	while (!surviving_attackers.empty() && !surviving_defenders.empty()) {
		int attacker_initiative = 0;
		int defender_initiative = 0;
		while (attacker_initiative == defender_initiative) {
			attacker_initiative = random::get()->roll_dice(initiative_dice);
			defender_initiative = random::get()->roll_dice(initiative_dice);
		}

		if (attacker_initiative < defender_initiative) {
			this->do_combat_round(surviving_attackers, surviving_defenders);
			this->do_combat_round(surviving_defenders, surviving_attackers);
		} else {
			this->do_combat_round(surviving_defenders, surviving_attackers);
			this->do_combat_round(surviving_attackers, surviving_defenders);
		}
	}

	return surviving_defenders.empty();
}

void game::do_combat_round(const std::vector<const character *> &characters, std::vector<const character *> &enemy_characters)
{
	if (characters.empty()) {
		return;
	}

	assert_throw(!enemy_characters.empty());

	for (const character *character : characters) {
		const metternich::character *chosen_enemy = vector::get_random(enemy_characters);

		static constexpr dice to_hit_dice(1, 20);
		const int to_hit = 20 - character->get_game_data()->get_to_hit_bonus();
		const int to_hit_result = to_hit - random::get()->roll_dice(to_hit_dice);

		const int armor_class_bonus = chosen_enemy->get_game_data()->get_armor_class_bonus() + chosen_enemy->get_game_data()->get_species_armor_class_bonus(character->get_species());
		const int armor_class = 10 - armor_class_bonus;
		if (to_hit_result > armor_class) {
			continue;
		}

		const int damage = random::get()->roll_dice(character->get_game_data()->get_damage_dice());
		chosen_enemy->get_game_data()->change_hit_points(-damage);

		if (chosen_enemy->get_game_data()->is_dead()) {
			std::erase(enemy_characters, chosen_enemy);
		}
	}
}

}
