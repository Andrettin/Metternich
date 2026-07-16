#include "metternich.h"

#include "game/game.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_history.h"
#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "culture/culture_history.h"
#include "database/database.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_parser.h"
#include "database/gsml_property.h"
#include "database/preferences.h"
#include "domain/diplomacy_state.h"
#include "domain/domain.h"
#include "domain/domain_ai.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "domain/domain_history.h"
#include "domain/domain_military.h"
#include "domain/domain_rank.h"
#include "domain/domain_technology.h"
#include "domain/domain_turn_data.h"
#include "domain/government_type.h"
#include "domain/office.h"
#include "economy/commodity.h"
#include "engine_interface.h"
#include "game/character_event.h"
#include "game/combat_base.h"
#include "game/domain_event.h"
#include "game/game_rules.h"
#include "game/province_event.h"
#include "game/scenario.h"
#include "game/site_event.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_type.h"
#include "infrastructure/holding_type.h"
#include "infrastructure/pathway.h"
#include "infrastructure/wonder.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/province_map_data.h"
#include "map/province_turn_data.h"
#include "map/region.h"
#include "map/region_history.h"
#include "map/route.h"
#include "map/route_game_data.h"
#include "map/route_history.h"
#include "map/site.h"
#include "map/site_feature.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/population_type.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/effect/delayed_effect_instance.h"
#include "technology/technology.h"
#include "time/calendar.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/civilian_unit_type.h"
#include "unit/historical_civilian_unit.h"
#include "unit/historical_civilian_unit_history.h"
#include "unit/historical_military_unit.h"
#include "unit/historical_military_unit_history.h"
#include "unit/historical_transporter.h"
#include "unit/historical_transporter_history.h"
#include "unit/military_unit.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/date_util.h"
#include "util/exception_util.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/path_util.h"
#include "util/random.h"
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
	connect(this, &game::current_combat_changed, this, &game::combat_running_changed);
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
	} else if (key == "scenario") {
		this->scenario = scenario::get(value);
	} else if (key == "player_character") {
		this->player_character = this->get_character(value);
	} else if (key == "player_country") {
		this->player_country = domain::get(value);
	} else {
		throw std::runtime_error(std::format("Invalid game data property: \"{}\".", key));
	}
}

void game::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "rules") {
		auto rules = make_qunique<game_rules>();
		scope.process(rules.get());
		this->rules = std::move(rules);
	} else if (tag == "map") {
		scope.process(map::get());
	} else if (tag == "domains") {
		scope.for_each_child([this](const gsml_data &domain_data) {
			domain *domain = domain::get(domain_data.get_tag());
			domain_data.process(domain->get_game_data());
			this->domains.push_back(domain);
		});
	} else if (tag == "countries") {
		for (const std::string &value : values) {
			this->countries.push_back(domain::get(value));
		}
	} else if (tag == "provinces") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			province *province = province::get(child_scope.get_tag());
			child_scope.process(province->get_game_data());

			map::get()->add_province(province);
		});
	} else if (tag == "sites") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const site *site = site::get(child_scope.get_tag());
			child_scope.process(site->get_game_data());

			map::get()->add_site(site);
		});
	} else if (tag == "routes") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const route *route = route::get(child_scope.get_tag());
			child_scope.process(route->get_game_data());
		});
	} else if (tag == "characters") {
		scope.for_each_child([this](const gsml_data &character_data) {
			const character *character = character::get(character_data.get_tag());
			character_data.process(character->get_game_data());
		});
	} else if (tag == "generated_characters") {
		scope.for_each_child([this](const gsml_data &character_data) {
			character_data.process(this->get_generated_character(character_data.get_tag()));
		});
	} else if (tag == "character_delayed_effects") {
		scope.for_each_child([this](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const character>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "country_delayed_effects") {
		scope.for_each_child([this](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const domain>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "province_delayed_effects") {
		scope.for_each_child([this](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const province>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "site_delayed_effects") {
		scope.for_each_child([this](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<const site>>();
			delayed_effect_data.process(delayed_effect.get());
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "fired_events") {
		scope.for_each_child([this](const gsml_data &fired_events_data) {
			const std::string &event_type_string = fired_events_data.get_tag();
			const std::vector<std::string> &child_values = fired_events_data.get_values();

			for (const std::string &child_value : child_values) {
				const metternich::event *event = nullptr;
				if (event_type_string == "character") {
					event = character_event::get(child_value);
				} else if (event_type_string == "domain") {
					event = domain_event::get(child_value);
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

	if (this->get_scenario() != nullptr) {
		data.add_property("scenario", this->get_scenario()->get_identifier());
	}

	gsml_data provinces_data("provinces");
	for (const province *province : map::get()->get_provinces()) {
		provinces_data.add_child(province->get_game_data()->to_gsml_data());
	}
	data.add_child(std::move(provinces_data));

	data.add_child("map", map::get()->to_gsml_data());

	gsml_data domains_data("domains");
	for (const domain *domain : this->get_domains()) {
		domains_data.add_child(domain->get_game_data()->to_gsml_data());
	}
	data.add_child(std::move(domains_data));

	gsml_data countries_data("countries");
	for (const domain *country : this->get_countries()) {
		countries_data.add_value(country->get_identifier());
	}
	data.add_child(std::move(countries_data));

	gsml_data sites_data("sites");
	for (const site *site : map::get()->get_sites()) {
		sites_data.add_child(site->get_game_data()->to_gsml_data());
	}
	data.add_child(std::move(sites_data));

	gsml_data routes_data("routes");
	for (const route *route : route::get_all()) {
		if (!route->get_game_data()->is_on_map()) {
			continue;
		}

		routes_data.add_child(route->get_game_data()->to_gsml_data());
	}
	data.add_child(std::move(routes_data));

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

QCoro::Task<void> game::load(const std::filesystem::path &filepath)
{
	try {
		if (!std::filesystem::exists(filepath)) {
			co_return;
		}

		if (this->is_running()) {
			co_await this->stop_coro();
		} else {
			co_await this->reset_game_data();
		}

		co_await this->initialize();

		gsml_parser parser;
		gsml_data data;

		try {
			data = parser.parse(filepath);
		} catch (...) {
			exception::report(std::current_exception());
			log::log_error(std::format("Failed to parse save file: {}", path::to_string(filepath)));
		}

		//create generated characters first, so that they can be referred to by objects when loading the saved game data
		data.get_child("generated_characters").for_each_child([this](const gsml_data &character_data) {
			auto generated_character = make_qunique<character>(character_data.get_tag());
			generated_character->moveToThread(QApplication::instance()->thread());
			this->add_generated_character(std::move(generated_character));
		});

		data.process(this);

		for (const domain *domain : this->get_domains()) {
			domain->get_game_data()->calculate_territory_rect();
		}

		co_await this->create_map_images();

		co_await this->start_coro();
	} catch (...) {
		exception::report(std::current_exception());
	}
}

QCoro::QmlTask game::load(const QUrl &filepath)
{
	return this->load(path::from_qurl(filepath));
}

QCoro::Task<void> game::setup_scenario_coro(const metternich::scenario *scenario)
{
	try {
		const metternich::scenario *old_scenario = this->scenario;

		co_await this->clear_coro();
		this->scenario = scenario;

		QDate start_date = scenario->get_start_date();
		if (scenario->get_start_date_calendar() != nullptr) {
			start_date = start_date.addYears(scenario->get_start_date_calendar()->get_year_offset());
		}

		this->date = game::normalize_date(start_date);

		database::get()->load_history(start_date, scenario->get_timeline(), this->get_rules());

		const map_template *map_template = scenario->get_map_template();
		if (old_scenario == nullptr || old_scenario->get_map_template() != map_template || map_template->is_randomly_generated()) {
			co_await map_template->apply();
			co_await map::get()->initialize(map_template->is_province_post_processing_enabled());

			//reset the game data for provinces and sites, since their constructors rely on the map having been initialized before
			co_await this->reset_game_data();
		}

		co_await this->initialize();
		co_await this->apply_history(start_date);

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

		std::vector<QFuture<void>> futures;
		for (const province *province : map::get()->get_provinces()) {
			QFuture<void> future = QtConcurrent::run([province]() {
				province->get_game_data()->create_map_image();
			});
			futures.push_back(std::move(future));
		}
		for (QFuture<void> &future : futures) {
			co_await future;
		}

		for (const site *site : map::get()->get_sites()) {
			assert_throw(site->get_game_data()->is_on_map());

			co_await site->get_game_data()->check_holding_type();
			co_await site->get_game_data()->check_employment();
			site->get_game_data()->calculate_commodity_outputs();
		}

		for (const domain *domain : this->get_domains()) {
			domain_game_data *domain_game_data = domain->get_game_data();
			domain_government *domain_government = domain->get_government();

			for (const office *office : office::get_all()) {
				co_await domain_government->check_office_holder(office);
			}

			domain_game_data->check_ideas();

			//setup journal entries, marking the ones for which the country already fulfills conditions as finished, but without doing the effects
			co_await domain_game_data->check_journal_entries(true, true);

			//ensure some item slots will start off filled
			domain_game_data->check_item_slots();

			//set commodity trade defaults
			for (const commodity *commodity : domain_game_data->get_economy()->get_tradeable_commodities()) {
				domain_game_data->get_economy()->set_default_min_commodity_storage(commodity);
				domain_game_data->get_economy()->set_default_max_commodity_storage(commodity);
			}
		}

		engine_interface::get()->reset_active_civilian_units();

		this->set_running(true);
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("Failed to start game.");
		QApplication::exit(EXIT_FAILURE);
	}
}

QCoro::Task<void> game::stop_coro()
{
	try {
		if (!this->is_running()) {
			//already stopped
			co_return;
		}

		this->set_running(false);
		co_await this->clear_coro();
		map::get()->clear();
		this->set_player_character(nullptr);
		this->set_player_country(nullptr);
	} catch (...) {
		exception::report(std::current_exception());
	}
}

QCoro::Task<void> game::clear_coro()
{
	try {
		this->rules = preferences::get()->get_game_rules()->duplicate();

		this->clear_delayed_effects();
		this->fired_events.clear();

		co_await this->reset_game_data();

		this->scenario = nullptr;
		this->domains.clear();
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

QCoro::Task<void> game::reset_game_data()
{
	//clear data related to the game (i.e. the data determined by history), but not that related only to the map
	//this is so that game setup speed can be faster if changing from one scenario to another with the same map template
	for (province *province : province::get_all()) {
		co_await province->reset_game_data();
	}

	for (site *site : site::get_all()) {
		site->reset_game_data();
	}

	for (domain *domain : domain::get_all()) {
		co_await domain->reset_game_data(false);
	}

	for (character *character : character::get_all()) {
		co_await character->reset_game_data();
	}

	this->generated_characters.clear();
}

QCoro::Task<void> game::initialize()
{
	for (const site *site : map::get()->get_sites()) {
		co_await site->get_game_data()->initialize();
	}
}

QCoro::Task<void> game::apply_history(const QDate &start_date)
{
	try {
		for (const region *region : region::get_all()) {
			region->get_history()->apply_to_provinces();
		}

		for (const province *province : map::get()->get_provinces()) {
			try {
				const province_history *province_history = province->get_history();
				province_game_data *province_game_data = province->get_game_data();

				if (province->is_water_zone()) {
					continue;
				}

				province_game_data->set_culture(province_history->get_main_culture());
				province_game_data->set_religion(province_history->get_religion());

				const pathway *pathway = province_history->get_pathway();
				for (const route *route : province->get_routes()) {
					const route_history *route_history = route->get_history();
					const metternich::pathway *route_pathway = route_history->get_pathway();
					if (route_pathway == nullptr) {
						continue;
					}

					if (pathway == nullptr || route_pathway->get_transport_level() > pathway->get_transport_level()) {
						pathway = route_pathway;
					}
				}
				if (pathway != nullptr) {
					co_await province_game_data->set_pathway(pathway);

					//add prerequisites for the province's pathway to its researched technologies
					if (pathway->get_required_technology() != nullptr) {
						co_await province_game_data->add_technology_with_prerequisites(pathway->get_required_technology());
					}

					const technology *terrain_required_technology = pathway->get_terrain_required_technology(province_game_data->get_terrain());
					if (terrain_required_technology != nullptr) {
						co_await province_game_data->add_technology_with_prerequisites(terrain_required_technology);
					}
				}

				for (const technology *technology : province_history->get_technologies()) {
					co_await province_game_data->add_technology_with_prerequisites(technology);
				}

				const culture *culture = province_game_data->get_culture();
				if (culture != nullptr) {
					const culture_history *culture_history = culture->get_history();
					co_await culture_history->apply_to_province(province);

					const cultural_group *cultural_group = culture->get_group();
					while (cultural_group != nullptr) {
						const metternich::culture_history *cultural_group_history = cultural_group->get_history();
						co_await cultural_group_history->apply_to_province(province);
						cultural_group = cultural_group->get_upper_group();
					}
				}

				const domain *owner = province_history->get_owner();
				if (owner != nullptr) {
					while (owner->get_history()->get_owner() != nullptr) {
						owner = owner->get_history()->get_owner();
					}
				}
				co_await province_game_data->set_owner(owner);

				if (owner == nullptr) {
					log::log_error(std::format("Province \"{}\" has no owner for scenario \"{}\".", province->get_identifier(), this->scenario->get_identifier()));
				}
			} catch (...) {
				std::throw_with_nested(std::runtime_error(std::format("Failed to apply history for province \"{}\".", province->get_identifier())));
			}
		}

		for (const site *site : map::get()->get_sites()) {
			site_game_data *site_game_data = site->get_game_data();
			const site_history *site_history = site->get_history();
			if (site_history->get_owner() != nullptr) {
				co_await site_game_data->set_owner(site_history->get_owner());
			}
		}

		for (const domain *domain : domain::get_all()) {
			domain->get_game_data()->apply_ruler_history(start_date);
		}

		for (const domain *domain : this->get_domains()) {
			co_await domain->get_game_data()->apply_history(start_date);
		}

		for (const domain *domain : this->get_domains()) {
			//diplomatic history has to be applied after main domain history application, so that tiers are correct when setting vassalage relationships
			co_await domain->get_game_data()->apply_diplomatic_history();
		}

		for (const cultural_group *cultural_group : cultural_group::get_all()) {
			const culture_history *culture_history = cultural_group->get_history();

			for (const domain *domain : this->get_domains()) {
				if (!domain->get_game_data()->get_culture()->is_part_of_group(cultural_group)) {
					continue;
				}

				co_await culture_history->apply_to_domain(domain);
			}
		}

		for (const culture *culture : culture::get_all()) {
			const culture_history *culture_history = culture->get_history();

			for (const domain *domain : this->get_domains()) {
				if (domain->get_game_data()->get_culture() != culture) {
					continue;
				}

				co_await culture_history->apply_to_domain(domain);
			}
		}

		for (const domain *domain : this->get_domains()) {
			domain_game_data *domain_game_data = domain->get_game_data();

			if (domain_game_data->get_overlord() != nullptr) {
				if (domain_game_data->get_subject_type() == nullptr) {
					throw std::runtime_error(std::format("Country \"{}\" is a vassal, but has no subject type.", domain->get_identifier()));
				}
			} else {
				if (domain_game_data->get_subject_type() != nullptr) {
					log::log_error(std::format("Country \"{}\" is not a vassal, but has a subject type.", domain->get_identifier()));

					co_await domain_game_data->set_subject_type(nullptr);
				}
			}

			co_await domain_game_data->check_government_type();
		}

		co_await this->apply_sites();

		for (const province *province : map::get()->get_provinces()) {
			for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
				site_game_data *settlement_game_data = settlement->get_game_data();
				if (settlement_game_data->is_built()) {
					co_await settlement_game_data->check_free_buildings();
				}
			}
		}

		co_await this->apply_free_on_start_buildings();
		co_await this->apply_population_history();

		//set stored commodities from history after the initial buildings have been constructed, so that buildings granting storage capacity (e.g. warehouses) will already be present
		for (const domain *domain : this->get_domains()) {
			for (const auto &[commodity, quantity] : domain->get_history()->get_commodities()) {
				domain->get_economy()->set_stored_commodity(commodity, quantity);
			}
		}

		co_await this->apply_character_history(start_date);

		/*
		for (const historical_civilian_unit *historical_civilian_unit : historical_civilian_unit::get_all()) {
			try {
				const historical_civilian_unit_history *historical_civilian_unit_history = historical_civilian_unit->get_history();

				if (!historical_civilian_unit_history->is_active()) {
					continue;
				}

				const province *province = historical_civilian_unit_history->get_province();

				assert_throw(province != nullptr);

				if (!province->get_game_data()->is_on_map()) {
					continue;
				}

				const domain *owner = historical_civilian_unit->get_owner();

				if (owner == nullptr && province != nullptr) {
					owner = province->get_game_data()->get_owner();
				}

				assert_throw(owner != nullptr);

				domain_game_data *owner_game_data = owner->get_game_data();

				assert_throw(owner_game_data->is_alive());

				if (owner_game_data->is_under_anarchy()) {
					continue;
				}

				const civilian_unit_type *type = historical_civilian_unit->get_type();
				assert_throw(type != nullptr);

				if (type->get_required_technology() != nullptr) {
					co_await province->get_game_data()->add_technology_with_prerequisites(type->get_required_technology());
				}

				const phenotype *phenotype = historical_civilian_unit->get_phenotype();
				const bool created = owner_game_data->create_civilian_unit(historical_civilian_unit->get_type(), province, phenotype);
				assert_throw(created);
			} catch (...) {
				std::throw_with_nested(std::runtime_error(std::format("Failed to apply historical civilian unit \"{}\".", historical_civilian_unit->get_identifier())));
			}
		}
		*/

		for (const site *site : map::get()->get_sites()) {
			//check employment because it could be necessary for providing manpower before applying military units
			co_await site->get_game_data()->check_employment();
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

				const domain *domain = historical_military_unit->get_domain();
				if (domain != nullptr && !domain->get_game_data()->is_alive()) {
					continue;
				}

				if (domain == nullptr) {
					domain = province->get_game_data()->get_owner();
				}

				assert_throw(domain != nullptr);

				domain_game_data *domain_game_data = domain->get_game_data();
				domain_military *domain_military = domain->get_military();

				assert_throw(domain_game_data->is_alive());

				const military_unit_type *type = historical_military_unit->get_type();
				assert_throw(type != nullptr);

				if (type->get_required_technology() != nullptr) {
					co_await province->get_game_data()->add_technology_with_prerequisites(type->get_required_technology());
				}

				const phenotype *phenotype = historical_military_unit->get_phenotype();

				for (int i = 0; i < historical_military_unit->get_quantity(); ++i) {
					const bool created = co_await domain_military->create_military_unit(type, province, phenotype, historical_military_unit_history->get_promotions());
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

			const domain *domain = historical_transporter->get_country();
			assert_throw(domain != nullptr);

			domain_game_data *domain_game_data = domain->get_game_data();
			domain_technology *domain_technology = domain->get_technology();

			if (!domain_game_data->is_alive()) {
				continue;
			}

			if (domain_game_data->is_under_anarchy()) {
				continue;
			}

			const transporter_type *type = historical_transporter->get_type();
			assert_throw(type != nullptr);

			if (type->get_required_technology() != nullptr) {
				co_await domain_technology->add_technology_with_prerequisites(type->get_required_technology());
			}

			const phenotype *phenotype = historical_transporter->get_phenotype();

			for (int i = 0; i < historical_transporter->get_quantity(); ++i) {
				const bool created = domain_game_data->create_transporter(type, phenotype);
				assert_throw(created);
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to apply history for scenario \"{}\".", this->scenario->get_identifier())));
	}
}

QCoro::Task<void> game::generate_site_resource_features()
{
	//generate province resource sites
	for (const province *province : map::get()->get_provinces()) {
		data_entry_map<site_feature, int> resource_counts = province->get_resource_counts();
		if (resource_counts.empty()) {
			continue;
		}

		//remove the already-existing resource sites from the counts
		for (const auto &[resource, count] : province->get_game_data()->get_site_feature_counts()) {
			resource_counts[resource] -= count;
		}

		std::vector<const site *> available_sites;
		for (const site *site : province->get_map_data()->get_sites()) {
			if (site->get_type() != site_type::holding) {
				continue;
			}

			available_sites.push_back(site);
		}

		if (available_sites.empty()) {
			continue;
		}

		co_await this->generate_site_resource_features(resource_counts, available_sites);
	}

	std::vector<region *> regions = region::get_all();

	std::sort(regions.begin(), regions.end(), [](const region *lhs, const region *rhs) {
		//give priority to smaller regions
		if (lhs->get_provinces().size() != rhs->get_provinces().size()) {
			return lhs->get_provinces().size() < rhs->get_provinces().size();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const region *region : regions) {
		data_entry_map<site_feature, int> resource_counts = region->get_resource_counts();
		if (resource_counts.empty()) {
			continue;
		}

		std::vector<const site *> available_sites;

		for (const province *province : region->get_provinces()) {
			//remove the already-existing resource sites from the counts
			for (const auto &[resource, count] : province->get_game_data()->get_site_feature_counts()) {
				resource_counts[resource] -= count;
			}

			for (const site *site : province->get_map_data()->get_sites()) {
				if (site->get_type() != site_type::holding) {
					continue;
				}

				available_sites.push_back(site);
			}
		}

		if (available_sites.empty()) {
			continue;
		}

		co_await this->generate_site_resource_features(resource_counts, available_sites);
	}
}

QCoro::Task<void> game::generate_site_resource_features(const data_entry_map<site_feature, int> &resource_counts, std::vector<const site *> &available_sites)
{
	//generate resource sites
	for (const auto &[resource, count] : resource_counts) {
		if (count <= 0) {
			continue;
		}

		assert_throw(resource->is_resource());

		std::vector<const site *> potential_resource_sites;

		for (const site *site : available_sites) {
			bool has_resource_feature = false;
			for (const site_feature *feature : site->get_game_data()->get_features()) {
				if (feature->is_resource()) {
					has_resource_feature = true;
					break;
				}
			}
			if (has_resource_feature) {
				continue;
			}

			if (!site->get_game_data()->can_have_feature(resource)) {
				continue;
			}

			potential_resource_sites.push_back(site);
		}

		for (int i = 0; i < count; ++i) {
			if (potential_resource_sites.empty()) {
				break;
			}

			const site *resource_site = vector::take_random(potential_resource_sites);
			co_await resource_site->get_game_data()->add_feature(resource);
			std::erase(available_sites, resource_site);
		}
	}
}

QCoro::Task<void> game::apply_sites()
{
	co_await this->generate_site_resource_features();

	for (const site *site : map::get()->get_sites()) {
		site_game_data *site_game_data = site->get_game_data();

		assert_throw(site->get_map_data()->is_on_map());

		//generate features for the start, if any are missing (e.g. resource features)
		//co_await site->get_game_data()->generate_features();

		const province *site_province = site->get_map_data()->get_province();
		assert_throw(site_province != nullptr);
		assert_throw(site_province->get_game_data()->is_on_map());

		const site_history *site_history = site->get_history();

		if (site->get_holding_type() != nullptr && site_history->is_developed()) {
			assert_throw(site_history->get_dungeon() == nullptr);

			co_await site_game_data->set_holding_type(site->get_holding_type());

			if (site_game_data->get_resource() != nullptr) {
				co_await map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
			}
		} else if (site_history->get_holding_type() != nullptr) {
			if (!site->is_settlement()) {
				throw std::runtime_error(std::format("Site \"{}\" has a holding type in history, but is not a holding.", site->get_identifier()));
			}

			assert_throw(site_history->get_dungeon() == nullptr);

			co_await site_game_data->set_holding_type(site_history->get_holding_type());

			if (site_game_data->get_resource() != nullptr) {
				co_await map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
			}
		} else if (site_history->get_dungeon() != nullptr) {
			co_await site_game_data->set_dungeon(site_history->get_dungeon());
		}
	}

	for (const site *site : map::get()->get_sites()) {
		site_game_data *site_game_data = site->get_game_data();

		if (site_game_data->get_holding_type() != nullptr) {
			assert_throw(site_game_data->get_owner() != nullptr);

			if (site_game_data->get_owner() != nullptr && !site_game_data->get_owner()->get_game_data()->get_government_type()->is_holding_type_allowed(site_game_data->get_holding_type())) {
				//for e.g. economic holding types, if the holding type is not allowed for the domain, see if there is a trade zone domain set in province history to hold it, and otherwise see if the province already has a calculated trade zone domain which can be used for this
				const province *site_province = site->get_map_data()->get_province();
				const province_history *province_history = site_province->get_history();
				const domain *province_trade_zone_domain = province_history->get_trade_zone() != nullptr ? province_history->get_trade_zone() : site_province->get_game_data()->get_trade_zone_domain();
				const domain *province_temple_domain = province_history->get_temple_domain() != nullptr ? province_history->get_temple_domain() : site_province->get_game_data()->get_temple_domain();

				if (province_trade_zone_domain != nullptr && site_game_data->get_holding_type()->is_economic()) {
					assert_throw(province_trade_zone_domain->get_game_data()->get_government_type()->is_holding_type_allowed(site_game_data->get_holding_type()));
					co_await site_game_data->set_owner(province_trade_zone_domain);
				} else if (province_temple_domain != nullptr && site_game_data->get_holding_type()->is_religious()) {
					assert_throw(province_temple_domain->get_game_data()->get_government_type()->is_holding_type_allowed(site_game_data->get_holding_type()));
					co_await site_game_data->set_owner(province_temple_domain);
				} else {
					//clear sites whose owner is not actually allowed to hold them
					log::log_error(std::format("Clearing holding site \"{}\", since its holding type (\"{}\") is not allowed for the government type (\"{}\") of its owner (\"{}\").", site->get_identifier(), site_game_data->get_holding_type()->get_identifier(), site_game_data->get_owner()->get_game_data()->get_government_type()->get_identifier(), site_game_data->get_owner()->get_identifier()));

					co_await site_game_data->set_holding_type(nullptr);
				}
			}
		}
	}

	//ensure provinces always have a built provincial capital
	for (const province *province : map::get()->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		province->get_game_data()->choose_provincial_capital();

		if (province->get_game_data()->get_provincial_capital() != nullptr) {
			continue;
		}

		//if no holding is built in the province, then ensure the best provincial capital slot is built
		const site *provincial_capital_slot = province->get_game_data()->get_best_provincial_capital_slot();

		if (provincial_capital_slot == nullptr) {
			log::log_error(std::format("Province \"{}\" has no suitable provincial capital slot.", province->get_identifier()));
			continue;
		}

		assert_throw(provincial_capital_slot->get_map_data()->is_on_map());

		if (provincial_capital_slot->get_game_data()->get_owner() != province->get_game_data()->get_owner()) {
			continue;
		}

		site_game_data *provincial_capital_slot_game_data = provincial_capital_slot->get_game_data();
		assert_throw(!provincial_capital_slot_game_data->is_built());

		const holding_type *best_holding_type = provincial_capital_slot->get_holding_type();
		assert_throw(best_holding_type != nullptr);
		co_await provincial_capital_slot_game_data->set_holding_type(best_holding_type);

		province->get_game_data()->choose_provincial_capital();
		assert_throw(province->get_game_data()->get_provincial_capital() != nullptr);
	}

	//set the capitals here, so that building requirements that require a capital can be fulfilled
	for (const domain *domain : this->get_domains()) {
		co_await domain->get_game_data()->set_capital(nullptr);
		co_await domain->get_game_data()->choose_capital();
	}

	std::vector<std::exception_ptr> exceptions;

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
				co_await this->apply_site_buildings(site);
			}

			if (tile == nullptr) {
				continue;
			}

			if (site_history->is_resource_discovered()) {
				assert_throw(site->get_type() == site_type::resource || site->get_type() == site_type::celestial_body || site->is_settlement());
				co_await map::get()->set_tile_resource_discovered(site_game_data->get_tile_pos(), true);
			}
		} catch (...) {
			try {
				std::throw_with_nested(std::runtime_error(std::format("Failed to apply history for site \"{}\".", site->get_identifier())));
			} catch (...) {
				exceptions.push_back(std::current_exception());
			}
		}
	}

	if (!exceptions.empty()) {
		throw aggregate_exception("Failed to apply site history.", std::move(exceptions));
	}

	//set province ratings based on holding levels
	for (const province *province : map::get()->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		std::map<const holding_type *, int> holding_type_levels;

		for (const site *holding_site : province->get_map_data()->get_settlement_sites()) {
			if (!holding_site->get_game_data()->is_built()) {
				continue;
			}

			holding_type_levels[holding_site->get_game_data()->get_holding_type()] += holding_site->get_game_data()->get_holding_level();
		}

		int highest_holding_level = 0;
		for (const auto &[holding_type, holding_level] : holding_type_levels) {
			assert_throw(holding_type != nullptr);
			highest_holding_level = std::max(highest_holding_level, holding_level);
		}

		province->get_game_data()->set_level(std::max(highest_holding_level, province->get_game_data()->get_level()));
	}
}

QCoro::Task<void> game::apply_site_buildings(const site *site)
{
	site_game_data *site_game_data = site->get_game_data();
	const tile *tile = site_game_data->get_tile();

	const province *site_province = site->get_province();
	if (site_province == nullptr && tile != nullptr) {
		site_province = tile->get_province();
	}

	const metternich::site *settlement = site->is_settlement() && tile != nullptr ? site : site_province->get_game_data()->get_provincial_capital();

	if (settlement == nullptr || !settlement->get_map_data()->is_on_map()) {
		co_return;
	}

	metternich::site_game_data *settlement_game_data = settlement->get_game_data();
	const metternich::holding_type *holding_type = settlement_game_data->get_holding_type();

	if (holding_type == nullptr) {
		co_return;
	}

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

	const domain *owner = settlement_game_data->get_owner();
	domain_game_data *owner_game_data = owner ? owner->get_game_data() : nullptr;

	for (auto [building_slot_type, building] : site_history->get_buildings()) {
		building_slot *building_slot = settlement_game_data->get_building_slot(building_slot_type);
		if (building_slot == nullptr) {
			if (settlement == site) {
				throw std::runtime_error(std::format("Holding \"{}\" is set in history to have building \"{}\", but does not have its building slot.", settlement->get_identifier(), building->get_identifier()));
			} else {
				continue;
			}
		}

		while (building != nullptr) {
			if (settlement == site) {
				if (holding_type == nullptr) {
					throw std::runtime_error(std::format("Holding \"{}\" is set in history to have building \"{}\", but has no holding type.", settlement->get_identifier(), building->get_identifier()));
				}

				if (!vector::contains(building->get_holding_types(), holding_type)) {
					throw std::runtime_error(std::format("Holding \"{}\" is set in history to have building \"{}\", but its holding type of \"{}\" is not appropriate for it.", settlement->get_identifier(), building->get_identifier(), holding_type->get_identifier()));
				}
			}

			if (building_slot->can_gain_building(building)) {
				break;
			}

			building = building->get_base_building();
		}

		if (building == nullptr) {
			continue;
		}

		const building_type *slot_building = settlement_game_data->get_slot_building(building_slot_type);

		if (slot_building == nullptr || slot_building->get_level() < building->get_level()) {
			co_await settlement_game_data->add_building_with_prerequisites(building);
		}
	}

	for (auto [building_slot_type, wonder] : site_history->get_wonders()) {
		if (!wonder->is_enabled()) {
			continue;
		}

		if (holding_type == nullptr) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but has no holding type.", settlement->get_identifier(), wonder->get_identifier()));
		}

		if (!vector::contains(wonder->get_building()->get_holding_types(), holding_type)) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but its holding type of \"{}\" is not appropriate for the wonder's building type of \"{}\".", settlement->get_identifier(), wonder->get_identifier(), holding_type->get_identifier(), wonder->get_building()->get_identifier()));
		}

		building_slot *building_slot = settlement_game_data->get_building_slot(building_slot_type);

		if (building_slot == nullptr) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but does not have its building slot available.", settlement->get_identifier(), wonder->get_identifier()));
		}

		if (!building_slot->can_have_wonder(wonder)) {
			throw std::runtime_error(std::format("Settlement \"{}\" is set in history to have wonder \"{}\", but cannot have it.", settlement->get_identifier(), wonder->get_identifier()));
		}

		co_await building_slot->set_wonder(wonder);

		if (wonder->get_required_technology() != nullptr && owner_game_data != nullptr) {
			co_await site_province->get_game_data()->add_technology_with_prerequisites(wonder->get_required_technology());
		}
	}

	const int holding_level = site_history->get_development_level();
	if (site_game_data->get_holding_type() != nullptr && holding_level != 0 && site_game_data->get_holding_level() < holding_level) {
		co_await site_game_data->set_holding_level_from_buildings(holding_level);
	}
}

QCoro::Task<void> game::apply_free_on_start_buildings()
{
	//build free on start buildings
	for (const site *site : map::get()->get_sites()) {
		if (!site->is_settlement()) {
			continue;
		}

		if (!site->get_game_data()->is_built()) {
			continue;
		}

		const province *province = site->get_game_data()->get_province();

		bool changed = true;
		while (changed) {
			changed = false;

			for (const building_type *building : site->get_game_data()->get_holding_type()->get_building_types()) {
				bool is_free_on_start = false;

				if (building->get_free_on_start_extra_technology().has_value()) {
					assert_throw(building->get_required_technology() != nullptr);
					const centesimal_int extra_technology = province->get_game_data()->get_extra_technology(building->get_required_technology());
					const centesimal_int extra_technology_threshold = building->get_free_on_start_extra_technology().value() + random::get()->generate_in_range(0, 1);
					if (extra_technology >= extra_technology_threshold) {
						is_free_on_start = true;
					}
				}

				if (!is_free_on_start && building->get_free_on_start_conditions() != nullptr && building->get_free_on_start_conditions()->check(site, read_only_context(site))) {
					is_free_on_start = true;
				}

				if (!is_free_on_start) {
					continue;
				}

				const bool added_building = co_await site->get_game_data()->check_free_building(building);
				if (added_building) {
					//loop through the buildings again, since the construction of another building might have made another possible to gain
					changed = true;
				}
			}
		}
	}
}

QCoro::Task<void> game::apply_population_history()
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

	domain_map<population_group_map<int64_t>> country_populations;

	for (const province *province : map::get()->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		province_game_data *province_game_data = province->get_game_data();

		if (province->get_history()->get_level() != 0) {
			province_game_data->set_level(province->get_history()->get_level());
		}

		if (province_game_data->get_settlement_count() == 0) {
			continue;
		}

		province->get_history()->distribute_population();

		population_group_map<int64_t> remaining_province_populations;

		for (const site *site : province_game_data->get_sites()) {
			site_game_data *site_game_data = site->get_game_data();

			if (!site_game_data->can_have_population()) {
				continue;
			}

			if (!site_game_data->is_built()) {
				continue;
			}

			site_history *site_history = site->get_history();

			site_history->initialize_population();

			for (const auto &[group_key, population] : site_history->get_population_groups()) {
				if (population <= 0) {
					continue;
				}

				const int64_t remaining_population = co_await this->apply_historical_population_group_to_site(group_key, population, site);

				if (remaining_population != 0 && site_game_data->get_owner() != nullptr) {
					//add the remaining population to remaining population data for the province
					remaining_province_populations[group_key] += remaining_population;
				}
			}

			if (site_game_data->get_population_units().empty()) {
				//ensure holdings have at least one population unit
				const int64_t population = 10000; //a bit of population to start with
				co_await this->apply_historical_population_group_to_site(population_group_key(), population, site);
			}
		}

		const site *provincial_capital = province->get_default_provincial_capital();
		assert_throw(provincial_capital != nullptr);

		for (const auto &[group_key, population] : remaining_province_populations) {
			if (population <= 0) {
				continue;
			}

			const int64_t remaining_population = co_await this->apply_historical_population_group_to_site(group_key, population, provincial_capital);

			if (remaining_population != 0 && provincial_capital->get_game_data()->get_owner() != nullptr) {
				//add the remaining population to remaining population data for the owner
				country_populations[provincial_capital->get_game_data()->get_owner()][group_key] += remaining_population;
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
		const population_group_map<int64_t> population_groups_copy = population_groups;
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

			const int64_t remaining_population = co_await this->apply_historical_population_group_to_site(group_key, population, capital);

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
		}
	}
}

QCoro::Task<int64_t> game::apply_historical_population_group_to_site(const population_group_key &group_key, const int64_t population, const site *site)
{
	if (population <= 0) {
		co_return 0;
	}

	site_game_data *site_game_data = site->get_game_data();

	if (!site_game_data->is_built()) {
		co_return population;
	}

	const population_type *population_type = group_key.type;
	if (population_type != nullptr) {
		if (!site_game_data->can_have_population_type(population_type)) {
			co_return population;
		}
	}

	if (site_game_data->get_available_population_capacity() == 0) {
		co_return population;
	}

	log_trace(std::format("Applying historical population group of type \"{}\", culture \"{}\", religion \"{}\" and size {} for settlement \"{}\".", population_type ? population_type->get_identifier() : "none", group_key.culture ? group_key.culture->get_identifier() : "none", group_key.religion ? group_key.religion->get_identifier() : "none", population, site->get_identifier()));

	const domain *domain = site_game_data->get_owner();

	if (domain == nullptr) {
		co_return 0;
	}

	site_history *site_history = site->get_history();

	const province *province = site->get_game_data()->get_province();
	province_history *province_history = province->get_history();

	const culture *site_culture = site_history->get_culture();

	int64_t remaining_population = population;

	const culture *culture = group_key.culture;
	culture_map<int64_t> culture_weights;
	if (culture == nullptr) {
		if (site_culture != nullptr) {
			culture = site_culture;
		} else if (!province_history->get_culture_weights().empty()) {
			culture_weights = province_history->get_culture_weights();
			if (culture_weights.size() == 1) {
				culture = culture_weights.begin()->first;
			}
		} else {
			log::log_error(std::format("Province \"{}\" has no culture weights.", province->get_identifier()));
			co_return 0;
		}
	}
	assert_throw(culture != nullptr || !culture_weights.empty());

	if (culture == nullptr) {
		assert_throw(!culture_weights.empty());

		int64_t new_remaining_population = remaining_population;
		const int64_t total_weight = archimedes::map::get_total_value(culture_weights);

		for (const auto &[weighted_culture, weight] : culture_weights) {
			population_group_key group_key_copy = group_key;
			group_key_copy.culture = weighted_culture;

			int64_t culture_population = remaining_population * weight / total_weight;
			culture_population -= co_await this->apply_historical_population_group_to_site(group_key_copy, culture_population, site);
			new_remaining_population -= culture_population;
		}

		remaining_population = new_remaining_population;

		if (remaining_population > 0) {
			//apply any remaining population to the biggest culture
			int64_t best_weight = 0;
			for (const auto &[weighted_culture, weight] : culture_weights) {
				if (weight > best_weight) {
					best_weight = weight;
					culture = weighted_culture;
				}
			}
		} else {
			co_return 0;
		}
	}
	assert_throw(culture != nullptr);

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
			co_return 0;
		}
	}
	assert_throw(religion != nullptr);

	const phenotype *phenotype = group_key.phenotype;
	phenotype_map<int64_t> phenotype_weights;
	if (phenotype == nullptr) {
		assert_throw(culture != nullptr);

		if (!province_history->get_phenotype_weights().empty()) {
			phenotype_weights = province_history->get_phenotype_weights_for_culture(culture);
		}

		if (phenotype_weights.empty()) {
			phenotype_weights = culture->get_phenotype_weights();
		}

		assert_throw(!phenotype_weights.empty());

		if (phenotype_weights.size() == 1) {
			phenotype = phenotype_weights.begin()->first;
		}
	}
	assert_throw(phenotype != nullptr || !phenotype_weights.empty());

	if (phenotype == nullptr) {
		assert_throw(!phenotype_weights.empty());

		int64_t new_remaining_population = remaining_population;
		const int64_t total_weight = archimedes::map::get_total_value(phenotype_weights);

		for (const auto &[weighted_phenotype, weight] : phenotype_weights) {
			population_group_key group_key_copy = group_key;
			group_key_copy.phenotype = weighted_phenotype;

			int64_t phenotype_population = remaining_population * weight / total_weight;
			phenotype_population -= co_await this->apply_historical_population_group_to_site(group_key_copy, phenotype_population, site);
			new_remaining_population -= phenotype_population;
		}

		remaining_population = new_remaining_population;

		if (remaining_population > 0) {
			//apply any remaining population to the biggest phenotype
			int64_t best_weight = 0;
			for (const auto &[weighted_phenotype, weight] : phenotype_weights) {
				if (weight > best_weight) {
					best_weight = weight;
					phenotype = weighted_phenotype;
				}
			}
		} else {
			co_return 0;
		}
	}
	assert_throw(phenotype != nullptr);

	assert_throw(culture != nullptr);
	assert_throw(religion != nullptr);
	assert_throw(phenotype != nullptr);

	if (population_type == nullptr) {
		population_type = site_game_data->get_default_population_type();
	}
	assert_throw(population_type != nullptr);

	decimillesimal_int literacy_rate = site_history->get_literacy_rate();
	if (literacy_rate == 0 && province_history->get_literacy_rate() > 0) {
		literacy_rate = province_history->get_literacy_rate();
	}
	if (literacy_rate == 0 && domain->get_history()->get_literacy_rate() > 0) {
		literacy_rate = domain->get_history()->get_literacy_rate();
	}

	const int64_t applied_population = std::min(remaining_population, site_game_data->get_available_population_capacity());

	co_await site_game_data->change_population(population_type, culture, religion, phenotype, nullptr, applied_population, literacy_rate, 0, true);

	co_return remaining_population - applied_population;
}

QCoro::Task<void> game::apply_character_history(const QDate &start_date)
{
	//sort characters by ancestry depth and birth date
	std::vector<character *> characters_by_birth_date = character::get_all();
	std::sort(characters_by_birth_date.begin(), characters_by_birth_date.end(), [](const character *lhs, const character *rhs) {
		if (lhs->get_ancestry_depth() != rhs->get_ancestry_depth()) {
			return lhs->get_ancestry_depth() < rhs->get_ancestry_depth();
		}

		if (lhs->get_game_data()->get_birth_date() != rhs->get_game_data()->get_birth_date()) {
			return lhs->get_game_data()->get_birth_date() < rhs->get_game_data()->get_birth_date();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	//apply character history
	for (const character *character : characters_by_birth_date) {
		character_game_data *character_game_data = character->get_game_data();
		co_await character_game_data->apply_history(start_date);
		if (character_game_data->is_dead()) {
			character->get_history()->calculate_heir();
		}
	}
}

QCoro::Task<void> game::on_setup_finished()
{
	for (const route *route : route::get_all()) {
		if (!route->get_game_data()->is_on_map()) {
			continue;
		}

		route->get_game_data()->check_active();
	}

	for (const domain *domain : this->get_domains()) {
		domain_game_data *domain_game_data = domain->get_game_data();
		//domain_economy *domain_economy = domain->get_economy();
		domain_government *domain_government = domain->get_government();

		co_await domain_game_data->check_government_type();
		co_await domain_government->check_laws();

		for (const office *office : office::get_all()) {
			co_await domain_government->check_office_holder(office);
		}

		domain_game_data->check_ideas();
		co_await domain_game_data->check_tier();
		co_await domain_game_data->check_culture();

		for (const province *province : domain_game_data->get_provinces()) {
			for (const site *site : province->get_game_data()->get_sites()) {
				//check employment here because it can affect the population type charts via equivalent population types for employment
				co_await site->get_game_data()->check_employment();
			}
		}

		//decrease population if there's too much for the starting food output
		/*
		while ((domain_economy->get_food_output() - domain_game_data->get_net_food_consumption()) < 0) {
			domain_game_data->decrease_population(false);
		}
		*/

		emit domain->game_data_changed();
	}

	co_await this->apply_free_on_start_buildings();

	this->calculate_domain_ranks();

	for (const domain *domain : this->get_domains()) {
		domain_game_data *domain_game_data = domain->get_game_data();
		domain_game_data->calculate_territory_rect();
	}

	co_await this->create_map_images();

	emit domains_changed();
	emit countries_changed();

	for (const character *character : character::get_all()) {
		character->get_game_data()->on_setup_finished();

		emit character->game_data_changed();
	}

	this->loaded = true;

	emit setup_finished();
}

QCoro::Task<void> game::do_turn_coro()
{
	try {
		co_await this->process_delayed_effects();

		domain_map<commodity_map<int64_t>> old_bids;
		domain_map<commodity_map<int64_t>> old_offers;

		for (province *province : map::get()->get_provinces()) {
			province->reset_turn_data();
		}

		for (domain *domain : this->get_domains()) {
			domain->reset_turn_data();

			domain->get_economy()->calculate_commodity_needs();

			if (domain->get_game_data()->is_ai()) {
				co_await domain->get_ai()->do_turn();
			}

			old_bids[domain] = domain->get_economy()->get_bids();
			old_offers[domain] = domain->get_economy()->get_offers();
		}

		for (const domain *domain : this->get_domains()) {
			co_await domain->get_game_data()->do_turn();

			domain->get_economy()->prepare_bids();
			domain->get_economy()->prepare_offers();
		}

		this->do_trade();

		for (const domain *domain : this->get_domains()) {
			//do country events after processing the turn for each country, so that e.g. events won't refer to a scope which no longer exists by the time the player gets to choose an option
			co_await domain->get_game_data()->do_events();

			//restore old bids and offers, if possible
			for (const auto &[commodity, bid] : old_bids[domain]) {
				domain->get_economy()->set_bid(commodity, bid);
			}

			for (const auto &[commodity, offer] : old_offers[domain]) {
				domain->get_economy()->set_offer(commodity, offer);
			}
		}

		if (!game::get()->get_player_country()->get_turn_data()->get_disbanded_military_units().empty()) {
			const portrait *war_minister_portrait = game::get()->get_player_country()->get_government()->get_war_minister_portrait();

			std::string disbanded_units_str;
			for (const auto &[military_unit_type, disbanded_count] : game::get()->get_player_country()->get_turn_data()->get_disbanded_military_units()) {
				disbanded_units_str += std::format("\n{} {}", disbanded_count, military_unit_type->get_name());
			}

			engine_interface::get()->add_notification("Military Units Disbanded", war_minister_portrait, std::format("{}, due to a lack of available resources, we were forced to disband some of our military units.\n\nDisbanded Units:{}", game::get()->get_player_country()->get_game_data()->get_form_of_address(), disbanded_units_str));
		}

		if (!game::get()->get_player_country()->get_turn_data()->get_province_spread_technologies().empty()) {
			const portrait *interior_minister_portrait = game::get()->get_player_country()->get_game_data()->get_government()->get_interior_minister_portrait();

			std::string spread_technologies_str;
			for (const auto &[province, spread_technologies] : game::get()->get_player_country()->get_turn_data()->get_province_spread_technologies()) {
				for (const technology *technology : spread_technologies) {
					spread_technologies_str += std::format("\n{} to {}", technology->get_name(), province->get_game_data()->get_current_cultural_name());
				}
			}

			engine_interface::get()->add_notification("Technology Spread", interior_minister_portrait, std::format("{}, new technologies have spread to our provinces!\n\nSpread Technologies:{}", game::get()->get_player_country()->get_game_data()->get_form_of_address(), spread_technologies_str));
		}

		for (const domain *domain : this->get_domains()) {
			if (domain->get_game_data()->get_province_count() == 0) {
				continue;
			}

			if (domain->get_turn_data()->is_diplomatic_map_dirty()) {
				domain->get_game_data()->create_diplomatic_map_image();
			} else {
				for (const diplomatic_map_mode mode : domain->get_turn_data()->get_dirty_diplomatic_map_modes()) {
					domain->get_game_data()->create_diplomatic_map_mode_image(mode);
				}

				for (const diplomacy_state state : domain->get_turn_data()->get_dirty_diplomatic_map_diplomacy_states()) {
					domain->get_game_data()->create_diplomacy_state_diplomatic_map_image(state);
				}
			}

			if (domain->get_turn_data()->is_realm_diplomatic_map_dirty()) {
				domain->get_game_data()->create_realm_diplomatic_map_image();
			}
		}

		for (const province *province : map::get()->get_provinces()) {
			if (province->get_turn_data()->is_province_map_dirty()) {
				province->get_game_data()->create_map_image();
			} else {
				const std::set<province_map_mode> dirty_map_modes = province->get_turn_data()->get_dirty_province_map_modes();
				for (const province_map_mode mode : dirty_map_modes) {
					province->get_game_data()->create_map_mode_image(mode);
				}
			}
		}

		if (this->exploration_changed) {
			co_await this->create_exploration_diplomatic_map_image();
			this->exploration_changed = false;
		}

		this->calculate_domain_ranks();

		this->increment_turn();
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("Failed to process turn.");
		QApplication::exit(EXIT_FAILURE);
	}
}

void game::do_trade()
{
	std::vector<metternich::domain *> trade_domains = this->get_domains();

	std::erase_if(trade_domains, [this](const domain *domain) {
		if (domain->get_game_data()->is_under_anarchy()) {
			return true;
		}

		return false;
	});

	std::sort(trade_domains.begin(), trade_domains.end(), [&](const metternich::domain *lhs, const metternich::domain *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const domain *domain : trade_domains) {
		domain->get_economy()->do_population_needs_purchasing();
	}

	for (const domain *domain : trade_domains) {
		domain->get_economy()->do_trade();
	}

	//change commodity prices based on whether there were unfulfilled bids/offers
	commodity_map<int64_t> remaining_demands;
	for (const domain *domain : trade_domains) {
		for (const auto &[commodity, bid] : domain->get_economy()->get_bids()) {
			remaining_demands[commodity] += bid;
		}

		for (const auto &[commodity, offer] : domain->get_economy()->get_offers()) {
			remaining_demands[commodity] -= offer;
		}
	}

	for (const auto &[commodity, value] : remaining_demands) {
		//change the price according to the extra quantity bid/offered
		const int64_t change = number::sqrt(std::abs(value)) * number::sign(value);

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

int64_t game::get_days_until_next_turn() const
{
	return this->get_date().daysTo(this->get_next_date());
}

bool game::is_last_turn_of_year() const
{
	return this->get_year() != this->get_next_date().year();
}

bool game::is_last_turn_of_quarter() const
{
	if (this->is_last_turn_of_year()) {
		return true;
	}

	return (this->get_date().month() - 1) / 3 != (this->get_next_date().month() - 1) / 3;
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
			return std::format("{}, {}", date::get_month_season_string(this->get_date().month()), this->year_to_labeled_string(this->get_year()));
		} else {
			return std::format("{}, {}", date::get_month_name(this->get_date().month()), this->year_to_labeled_string(this->get_year()));
		}
	} else {
		return this->year_to_labeled_string(this->get_year());
	}
}

std::string game::year_to_labeled_string(const int year) const
{
	//FIXME: add support for different calendars depending on the domain/culture being played
	return date::year_to_labeled_string(year);
}

std::string game::year_range_to_labeled_string(const int start_year, const int end_year) const
{
	//FIXME: add support for different calendars depending on the domain/culture being played
	return date::year_range_to_labeled_string(start_year, end_year);
}

QVariantList game::get_domains_qvariant_list() const
{
	return container::to_qvariant_list(this->get_domains());
}

void game::add_domain(domain *domain)
{
	this->domains.push_back(domain);

	if (this->is_running()) {
		emit domains_changed();
	}
}

QCoro::Task<void> game::remove_domain(domain *domain)
{
	for (const character *character : domain->get_game_data()->get_characters()) {
		character->get_game_data()->set_domain(nullptr);
	}

	domain->get_military()->clear_leaders();

	for (const metternich::domain *other_domain : this->get_domains()) {
		domain_game_data *other_domain_game_data = other_domain->get_game_data();

		if (other_domain_game_data->get_diplomacy_state(domain) != diplomacy_state::peace) {
			co_await other_domain_game_data->set_diplomacy_state(domain, diplomacy_state::peace);
			co_await domain->get_game_data()->set_diplomacy_state(other_domain, diplomacy_state::peace);
		}

		if (other_domain_game_data->get_consulate(domain) != nullptr) {
			other_domain_game_data->set_consulate(domain, nullptr);
		}
	}

	std::erase(this->domains, domain);

	if (this->is_running()) {
		emit domains_changed();
	}

	co_await domain->reset_game_data(true);
}

QVariantList game::get_countries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_countries());
}

void game::add_country(domain *domain)
{
	this->countries.push_back(domain);

	if (this->is_running()) {
		emit countries_changed();
	}
}

void game::remove_country(domain *domain)
{
	std::erase(this->countries, domain);

	if (this->is_running()) {
		emit countries_changed();
	}
}

void game::calculate_domain_ranks()
{
	std::vector<metternich::domain *> domains = game::get()->get_domains();

	if (domains.empty()) {
		return;
	}

	std::sort(domains.begin(), domains.end(), [](const metternich::domain *lhs, const metternich::domain *rhs) {
		if (lhs->get_game_data()->is_under_anarchy() != rhs->get_game_data()->is_under_anarchy()) {
			return rhs->get_game_data()->is_under_anarchy();
		}

		return lhs->get_game_data()->get_score() > rhs->get_game_data()->get_score();
	});

	int64_t average_score = 0;
	int highest_score = domains.at(0)->get_game_data()->get_score();
	
	for (size_t i = 0; i < domains.size(); ++i) {
		const domain *domain = domains.at(i);
		domain->get_game_data()->set_score_rank(static_cast<int>(i));

		average_score += domain->get_game_data()->get_score();
	}

	average_score /= domains.size();

	for (const domain *domain : domains) {
		const domain_rank *best_rank = nullptr;
		for (const domain_rank *rank : domain_rank::get_all()) {
			if (best_rank != nullptr && best_rank->get_priority() >= rank->get_priority()) {
				continue;
			}

			if (rank->get_conditions() != nullptr && !rank->get_conditions()->check(domain)) {
				continue;
			}

			const centesimal_int score(domain->get_game_data()->get_score());
			const centesimal_int average_score_threshold = rank->get_average_score_threshold() * average_score;
			const centesimal_int relative_score_threshold = rank->get_relative_score_threshold() * highest_score;
			if (score >= average_score_threshold || score >= relative_score_threshold) {
				best_rank = rank;
			}
		}

		if (best_rank != nullptr) {
			domain->get_game_data()->set_rank(best_rank);
		}
	}
}

void game::set_player_country(const domain *domain)
{
	if (domain == this->get_player_country()) {
		return;
	}

	this->player_country = domain;

	if (this->is_running()) {
		emit player_country_changed();
	}

	this->set_player_character(domain ? domain->get_government()->get_ruler() : nullptr);
}

int64_t game::get_price(const commodity *commodity) const
{
	/*
	const auto find_iterator = this->prices.find(commodity);

	if (find_iterator != this->prices.end()) {
		return find_iterator->second;
	}
	*/

	return commodity->get_base_price();
}

void game::set_price(const commodity *commodity, const int64_t value)
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

QCoro::Task<void> game::create_map_images()
{
	co_await this->create_diplomatic_map_image();
}

QCoro::Task<void> game::create_diplomatic_map_image()
{
	std::vector<QCoro::Task<void>> tasks;
	if (map::get()->get_ocean_diplomatic_map_image().isNull()) {
		QCoro::Task<void> task = map::get()->create_ocean_diplomatic_map_image();
		tasks.push_back(std::move(task));
	}

	std::vector<QFuture<void>> futures;
	for (const domain *domain : this->get_domains()) {
		QFuture<void> future = QtConcurrent::run([domain]() {
			domain_game_data *domain_game_data = domain->get_game_data();
			domain_game_data->create_diplomatic_map_image();
			domain_game_data->create_realm_diplomatic_map_image();
		});
		futures.push_back(std::move(future));
	}

	for (QCoro::Task<void> &task : tasks) {
		co_await std::move(task);
	}

	for (QFuture<void> &future : futures) {
		co_await future;
	}
}

QCoro::Task<void> game::create_exploration_diplomatic_map_image()
{
	if (!map::exploration_enabled) {
		this->exploration_diplomatic_map_image = QImage(QSize(1, 1), QImage::Format_RGBA8888);
		this->exploration_diplomatic_map_image.fill(Qt::transparent);
		co_return;
	}

	const map *map = map::get();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	QSize image_size;
	if (tile_scale < 1) {
		image_size = map::get()->get_size() * tile_scale;
	} else {
		image_size = map->get_size();
	}

	this->exploration_diplomatic_map_image = QImage(image_size, QImage::Format_RGBA8888);
	this->exploration_diplomatic_map_image.fill(Qt::transparent);

	const QColor &color = defines::get()->get_unexplored_terrain()->get_color();

	const domain_game_data *domain_game_data = this->get_player_country()->get_game_data();

	for (int x = 0; x < this->exploration_diplomatic_map_image.width(); ++x) {
		for (int y = 0; y < this->exploration_diplomatic_map_image.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;

			if (domain_game_data->is_tile_explored(tile_pos)) {
				continue;
			}

			this->exploration_diplomatic_map_image.setPixelColor(pixel_pos, color);
		}
	}

	if (tile_scale > 1) {
		QImage scaled_exploration_diplomatic_map_image;

		co_await QtConcurrent::run([this, tile_scale, &scaled_exploration_diplomatic_map_image]() {
			scaled_exploration_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(this->exploration_diplomatic_map_image, centesimal_int(tile_scale), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
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
	}

	assert_throw(this->exploration_diplomatic_map_image.size() == map::get()->get_diplomatic_map_image_size());
	assert_throw(this->exploration_diplomatic_map_image.size
	() == map::get()->get_ocean_diplomatic_map_image().size());
}

const character *game::get_character(const std::string &identifier) const
{
	const character *character = character::try_get(identifier);
	if (character != nullptr) {
		return character;
	}

	return this->get_generated_character(identifier);
}

const character *game::get_generated_character(const std::string &identifier) const
{
	const auto find_iterator = this->generated_characters_by_identifier.find(identifier);
	if (find_iterator != this->generated_characters_by_identifier.end()) {
		return find_iterator->second;
	}

	throw std::runtime_error(std::format("Failed to get game character for identifier \"{}\".", identifier));
}

character *game::get_generated_character(const std::string &identifier)
{
	const auto find_iterator = this->generated_characters_by_identifier.find(identifier);
	if (find_iterator != this->generated_characters_by_identifier.end()) {
		return find_iterator->second;
	}

	throw std::runtime_error(std::format("Failed to get game character for identifier \"{}\".", identifier));
}

void game::add_generated_character(qunique_ptr<character> &&character)
{
	this->generated_characters_by_identifier[character->get_identifier()] = character.get();
	this->generated_characters.push_back(std::move(character));
}

void game::remove_generated_character(character *character)
{
	this->generated_characters_by_identifier.erase(character->get_identifier());
	vector::remove(this->generated_characters, character);
}

QCoro::Task<void> game::process_delayed_effects()
{
	co_await this->process_delayed_effects(this->character_delayed_effects);
	co_await this->process_delayed_effects(this->country_delayed_effects);
	co_await this->process_delayed_effects(this->province_delayed_effects);
	co_await this->process_delayed_effects(this->site_delayed_effects);
}

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<const character>> &&delayed_effect)
{
	this->character_delayed_effects.push_back(std::move(delayed_effect));
}

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<const domain>> &&delayed_effect)
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

QCoro::Task<bool> game::do_battle(army *attacking_army, army *defending_army)
{
	//this function returns true if the attackers won, or false otherwise

	static constexpr dice battle_dice(1, 6);

	military_unit_type_map<int> lost_unit_count;
	//keep track of the player's starting unit count, so that we can later calculate their lost unit count
	if (attacking_army->get_domain() == this->get_player_country()) {
		for (const military_unit *military_unit : attacking_army->get_military_units()) {
			lost_unit_count[military_unit->get_type()]++;
		}
	} else if (defending_army->get_domain() == this->get_player_country()) {
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
				co_await defending_unit->change_hit_points(-1);
				--attacker_hits;
				if (attacker_hits == 0) {
					break;
				}
			}
		}
		
		while (defender_hits > 0 && !attacking_army->get_military_units().empty()) {
			const std::vector<military_unit *> attacking_units = attacking_army->get_military_units();
			for (military_unit *attacking_unit : attacking_units) {
				co_await attacking_unit->change_hit_points(-1);
				--defender_hits;
				if (defender_hits == 0) {
					break;
				}
			}
		}
	}

	//restore hit points of the surviving units
	for (military_unit *attacking_unit : attacking_army->get_military_units()) {
		co_await attacking_unit->fully_recover();
	}
	for (military_unit *defending_unit : defending_army->get_military_units()) {
		co_await defending_unit->fully_recover();
	}

	assert_throw(attacking_army->get_military_units().empty() || defending_army->get_military_units().empty());

	const bool attack_success = defending_army->get_military_units().empty() && !attacking_army->get_military_units().empty();

	//display a notification for the player about the battle
	if (attacking_army->get_domain() == this->get_player_country() || defending_army->get_domain() == this->get_player_country()) {
		const bool is_attacker = attacking_army->get_domain() == this->get_player_country();
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

	co_return attack_success;
}

void game::set_current_combat(qunique_ptr<combat_base> &&combat)
{
	this->current_combat = std::move(combat);

	emit current_combat_changed();
}

}
