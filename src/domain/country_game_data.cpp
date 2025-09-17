#include "metternich.h"

#include "domain/country_game_data.h"

#include "character/character.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "domain/consulate.h"
#include "domain/country.h"
#include "domain/country_ai.h"
#include "domain/country_economy.h"
#include "domain/country_government.h"
#include "domain/country_military.h"
#include "domain/country_rank.h"
#include "domain/country_technology.h"
#include "domain/country_tier.h"
#include "domain/country_tier_data.h"
#include "domain/country_turn_data.h"
#include "domain/country_type.h"
#include "domain/culture.h"
#include "domain/diplomacy_state.h"
#include "domain/government_type.h"
#include "domain/idea.h"
#include "domain/idea_slot.h"
#include "domain/idea_trait.h"
#include "domain/idea_type.h"
#include "domain/journal_entry.h"
#include "domain/subject_type.h"
#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"
#include "economy/income_transaction_type.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/improvement_slot.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/settlement_type.h"
#include "infrastructure/wonder.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "religion/deity.h"
#include "religion/deity_slot.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "script/opinion_modifier.h"
#include "script/scripted_country_modifier.h"
#include "species/phenotype.h"
#include "technology/research_organization.h"
#include "technology/research_organization_slot.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/civilian_unit.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit.h"
#include "unit/military_unit_type.h"
#include "unit/transporter.h"
#include "unit/transporter_class.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/point_util.h"
#include "util/qunique_ptr.h"
#include "util/rect_util.h"
#include "util/size_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

#include "xbrz.h"

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_utility.hpp>

namespace metternich {

country_game_data::country_game_data(metternich::country *country)
	: country(country), tier(country_tier::none), religion(country->get_default_religion())
{
	this->economy = make_qunique<country_economy>(country, this);
	this->government = make_qunique<country_government>(country, this);
	this->military = make_qunique<country_military>(country);
	this->technology = make_qunique<country_technology>(country, this);

	connect(this, &country_game_data::tier_changed, this, &country_game_data::title_name_changed);
	connect(this->get_government(), &country_government::government_type_changed, this, &country_game_data::title_name_changed);
	connect(this, &country_game_data::religion_changed, this, &country_game_data::title_name_changed);
	connect(this, &country_game_data::rank_changed, this, &country_game_data::type_name_changed);

	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::type_count_changed, this, &country_game_data::on_population_type_count_changed);

	connect(this, &country_game_data::provinces_changed, this, &country_game_data::income_changed);
	connect(this, &country_game_data::provinces_changed, this, &country_game_data::maintenance_cost_changed);
	connect(this->get_military(), &country_military::military_units_changed, this, &country_game_data::maintenance_cost_changed);
}

country_game_data::~country_game_data()
{
}

void country_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "tier") {
		this->tier = magic_enum::enum_cast<country_tier>(value).value();
	} else if (key == "religion") {
		this->religion = religion::get(value);
	} else {
		throw std::runtime_error(std::format("Invalid country game data property: \"{}\".", key));
	}
}

void country_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	throw std::runtime_error(std::format("Invalid country game data scope: \"{}\".", tag));
}

gsml_data country_game_data::to_gsml_data() const
{
	gsml_data data(this->country->get_identifier());

	data.add_property("tier", std::string(magic_enum::enum_name(this->get_tier())));
	data.add_property("religion", this->get_religion()->get_identifier());

	return data;
}

void country_game_data::do_turn()
{
	try {
		for (const province *province : this->get_provinces()) {
			province->get_game_data()->do_turn();
		}

		this->get_economy()->do_production();
		this->collect_wealth();
		this->pay_maintenance();
		this->do_transporter_recruitment();
		this->do_civilian_unit_recruitment();
		this->get_military()->do_military_unit_recruitment();
		this->get_technology()->do_research();
		this->do_population_growth();
		this->do_cultural_change();

		for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
			civilian_unit->do_turn();
		}

		for (const qunique_ptr<military_unit> &military_unit : this->get_military()->get_military_units()) {
			military_unit->do_turn();
		}

		for (const qunique_ptr<transporter> &transporter : this->transporters) {
			transporter->do_turn();
		}

		for (const qunique_ptr<army> &army : this->get_military()->get_armies()) {
			army->do_turn();
		}

		this->get_military()->clear_armies();

		this->decrement_scripted_modifiers();

		this->check_journal_entries();

		this->get_government()->check_government_type();
		this->check_characters();
		this->check_ideas();
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to process turn for country \"{}\".", this->country->get_identifier())));
	}
}

void country_game_data::collect_wealth()
{
	int collected_taxes = 0;

	for (const province *province : this->get_provinces()) {
		collected_taxes += province->get_game_data()->collect_taxes();
	}

	this->country->get_turn_data()->add_income_transaction(income_transaction_type::taxation, collected_taxes, nullptr, 0, this->country);
}

void country_game_data::pay_maintenance()
{
	const int domain_maintenance_cost = std::min(this->get_domain_maintenance_cost(), this->get_economy()->get_stored_commodity(defines::get()->get_wealth_commodity()));
	if (domain_maintenance_cost != 0) {
		this->get_economy()->change_stored_commodity(defines::get()->get_wealth_commodity(), -domain_maintenance_cost);

		this->country->get_turn_data()->add_expense_transaction(expense_transaction_type::domain_maintenance, domain_maintenance_cost, nullptr, 0, this->country);
	}

	std::vector<military_unit *> military_units_to_disband;

	for (const qunique_ptr<military_unit> &military_unit : this->get_military()->get_military_units()) {
		for (const auto &[commodity, maintenance_cost] : military_unit->get_type()->get_maintenance_commodity_costs()) {
			if (this->get_economy()->get_stored_commodity(commodity) >= maintenance_cost) {
				this->get_economy()->change_stored_commodity(commodity, -maintenance_cost);

				if (commodity == defines::get()->get_wealth_commodity()) {
					this->country->get_turn_data()->add_expense_transaction(expense_transaction_type::military_maintenance, maintenance_cost, nullptr, 0, this->country);
				} else {
					this->country->get_turn_data()->add_expense_transaction(expense_transaction_type::military_maintenance, 0, commodity, maintenance_cost, this->country);
				}
			} else {
				military_units_to_disband.push_back(military_unit.get());
			}
		}
	}

	military_unit_type_map<int> disbanded_type_counts;
	for (military_unit *military_unit : military_units_to_disband) {
		disbanded_type_counts[military_unit->get_type()]++;
		military_unit->disband(false);
	}

	if (!disbanded_type_counts.empty() && this->country == game::get()->get_player_country()) {
		const portrait *war_minister_portrait = this->get_government()->get_war_minister_portrait();

		std::string disbanded_units_str;
		for (const auto &[military_unit_type, disbanded_count] : disbanded_type_counts) {
			disbanded_units_str += std::format("\n{} {}", disbanded_count, military_unit_type->get_name());
		}

		engine_interface::get()->add_notification("Military Units Disbanded", war_minister_portrait, std::format("Your Excellency, our finances are in dire straits! Due to a lack of available resources, we were forced to disband some of our military units.\n\nDisbanded Units:{}", disbanded_units_str));
	}
}

void country_game_data::do_civilian_unit_recruitment()
{
	try {
		if (this->is_under_anarchy()) {
			return;
		}

		const data_entry_map<civilian_unit_type, int> recruitment_counts = this->civilian_unit_recruitment_counts;
		for (const auto &[civilian_unit_type, recruitment_count] : recruitment_counts) {
			assert_throw(recruitment_count > 0);

			for (int i = 0; i < recruitment_count; ++i) {
				const bool created = this->create_civilian_unit(civilian_unit_type, nullptr, nullptr);
				const bool restore_costs = !created;
				this->change_civilian_unit_recruitment_count(civilian_unit_type, -1, restore_costs);
			}
		}

		assert_throw(this->civilian_unit_recruitment_counts.empty());
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing civilian unit recruitment for country \"{}\".", this->country->get_identifier())));
	}
}

void country_game_data::do_transporter_recruitment()
{
	try {
		if (this->is_under_anarchy()) {
			return;
		}

		const transporter_type_map<int> recruitment_counts = this->transporter_recruitment_counts;
		for (const auto &[transporter_type, recruitment_count] : recruitment_counts) {
			assert_throw(recruitment_count > 0);

			for (int i = 0; i < recruitment_count; ++i) {
				const bool created = this->create_transporter(transporter_type, nullptr);
				const bool restore_costs = !created;
				this->change_transporter_recruitment_count(transporter_type, -1, restore_costs);
			}
		}

		assert_throw(this->transporter_recruitment_counts.empty());
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing transporter recruitment for country \"{}\".", this->country->get_identifier())));
	}
}

void country_game_data::do_population_growth()
{
	try {
		if (this->get_food_consumption() == 0) {
			this->set_population_growth(0);
			return;
		}

		const int available_food = this->get_available_food();
		const int available_housing = std::max(0, this->get_available_housing().to_int());

		int food_consumption = this->get_net_food_consumption();

		const int population_growth_change = std::min(available_food, available_housing);
		this->change_population_growth(population_growth_change);

		if (population_growth_change > 0) {
			//food consumed for population growth
			food_consumption += population_growth_change;
		}
		this->do_food_consumption(food_consumption);

		while (this->get_population_growth() >= defines::get()->get_population_growth_threshold()) {
			this->grow_population();
		}

		if (this->get_population_growth() < 0) {
			this->do_starvation();
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing population growth for country \"{}\".", this->country->get_identifier())));
	}
}

void country_game_data::do_food_consumption(const int food_consumption)
{
	int remaining_food_consumption = food_consumption;

	//this is a copy because we may need to erase elements from the map in the subsequent code
	const commodity_map<int> stored_commodities = this->get_economy()->get_stored_commodities();

	for (const auto &[commodity, quantity] : stored_commodities) {
		if (commodity->is_food()) {
			const int consumed_food = std::min(remaining_food_consumption, quantity);
			this->get_economy()->change_stored_commodity(commodity, -consumed_food);

			remaining_food_consumption -= consumed_food;

			if (remaining_food_consumption == 0) {
				break;
			}
		}
	}
}

void country_game_data::do_starvation()
{
	int starvation_count = 0;

	while (this->get_population_growth() < 0) {
		//starvation
		this->decrease_population(true);
		++starvation_count;

		if (this->get_food_consumption() == 0) {
			this->set_population_growth(0);
			break;
		}
	}

	if (starvation_count > 0 && this->country == game::get()->get_player_country()) {
		const bool plural = starvation_count > 1;

		const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

		engine_interface::get()->add_notification("Starvation", interior_minister_portrait, std::format("Your Excellency, I regret to inform you that {} {} of our population {} starved to death.", number::to_formatted_string(starvation_count), (plural ? "units" : "unit"), (plural ? "have" : "has")));
	}
}

void country_game_data::do_cultural_change()
{
	static constexpr int base_cultural_derivation_chance = 1;

	for (population_unit *population_unit : this->population_units) {
		const metternich::culture *current_culture = population_unit->get_culture();

		std::vector<const metternich::culture *> potential_cultures;

		const read_only_context ctx(population_unit);

		for (const metternich::culture *culture : current_culture->get_derived_cultures()) {
			if (culture->get_derivation_conditions() != nullptr && !culture->get_derivation_conditions()->check(population_unit, ctx)) {
				continue;
			}

			potential_cultures.push_back(culture);
		}

		if (potential_cultures.empty()) {
			continue;
		}

		vector::shuffle(potential_cultures);

		for (const metternich::culture *culture : potential_cultures) {
			int chance = base_cultural_derivation_chance;

			if (this->country->get_culture() == culture) {
				chance *= 2;
			}

			if (random::get()->generate(100) >= chance) {
				continue;
			}

			const metternich::culture *new_culture = vector::get_random(potential_cultures);
			population_unit->set_culture(new_culture);
			break;
		}
	}
}

void country_game_data::do_events()
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_events();
	}

	if (this->is_under_anarchy()) {
		return;
	}

	const bool is_last_turn_of_year = game::get()->get_year() != game::get()->get_next_date().year();
	if (is_last_turn_of_year) {
		country_event::check_events_for_scope(this->country, event_trigger::yearly_pulse);
	}

	const bool is_last_turn_of_quarter = is_last_turn_of_year || (game::get()->get_date().month() - 1) / 4 != (game::get()->get_next_date().month() - 1) / 4;
	if (is_last_turn_of_quarter) {
		country_event::check_events_for_scope(this->country, event_trigger::quarterly_pulse);
	}

	country_event::check_events_for_scope(this->country, event_trigger::per_turn_pulse);
}

bool country_game_data::is_ai() const
{
	return this->country != game::get()->get_player_country();
}

country_ai *country_game_data::get_ai() const
{
	return this->country->get_ai();
}

void country_game_data::set_tier(const country_tier tier)
{
	if (tier == this->get_tier()) {
		return;
	}

	assert_throw(tier >= this->country->get_min_tier());
	assert_throw(tier <= this->country->get_max_tier());

	if (this->get_tier() != country_tier::none) {
		const country_tier_data *tier_data = country_tier_data::get(this->get_tier());
		if (tier_data->get_modifier() != nullptr) {
			tier_data->get_modifier()->remove(this->country);
		}
	}

	this->tier = tier;

	if (this->get_tier() != country_tier::none) {
		const country_tier_data *tier_data = country_tier_data::get(this->get_tier());
		if (tier_data->get_modifier() != nullptr) {
			tier_data->get_modifier()->apply(this->country);
		}
	}

	if (game::get()->is_running()) {
		emit tier_changed();
	}
}

const std::string &country_game_data::get_name() const
{
	return this->country->get_name(this->get_government()->get_government_type(), this->get_tier());
}

std::string country_game_data::get_titled_name() const
{
	return this->country->get_titled_name(this->get_government()->get_government_type(), this->get_tier(), this->get_religion());
}

const std::string &country_game_data::get_title_name() const
{
	return this->country->get_title_name(this->get_government()->get_government_type(), this->get_tier(), this->get_religion());
}

void country_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->religion = religion;

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->get_population()->get_main_religion() == nullptr) {
			province->get_game_data()->set_religion(this->get_religion());
		}
	}

	this->get_technology()->check_technologies();

	if (game::get()->is_running()) {
		emit religion_changed();
	}
}

void country_game_data::set_overlord(const metternich::country *overlord)
{
	if (overlord == this->get_overlord()) {
		return;
	}

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(-this->get_economic_score() * country_game_data::vassal_tax_rate / 100);

		for (const auto &[resource, count] : this->get_economy()->get_resource_counts()) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, -count);
		}
	}

	this->overlord = overlord;

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(this->get_economic_score() * country_game_data::vassal_tax_rate / 100);

		for (const auto &[resource, count] : this->get_economy()->get_resource_counts()) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, count);
		}
	} else {
		this->set_subject_type(nullptr);
	}

	if (game::get()->is_running()) {
		emit overlord_changed();
	}
}

bool country_game_data::is_vassal_of(const metternich::country *country) const
{
	return this->get_overlord() == country;
}

bool country_game_data::is_any_vassal_of(const metternich::country *country) const
{
	if (this->is_vassal_of(country)) {
		return true;
	}

	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->is_any_vassal_of(country);
	}

	return false;
}

bool country_game_data::is_overlord_of(const metternich::country *country) const
{
	return country->get_game_data()->is_vassal_of(this->country);
}

bool country_game_data::is_any_overlord_of(const metternich::country *country) const
{
	if (this->is_overlord_of(country)) {
		return true;
	}

	const std::vector<const metternich::country *> vassals = this->get_vassals();
	for (const metternich::country *vassal : this->get_vassals()) {
		if (vassal->get_game_data()->is_any_overlord_of(country)) {
			return true;
		}
	}

	return false;
}

std::string country_game_data::get_type_name() const
{
	switch (this->country->get_type()) {
		case country_type::polity:
			if (this->get_rank() != nullptr) {
				return this->get_rank()->get_name();
			}
		case country_type::clade:
		case country_type::tribe:
			return get_country_type_name(this->country->get_type());
		default:
			assert_throw(false);
	}

	return std::string();
}


void country_game_data::set_subject_type(const metternich::subject_type *subject_type)
{
	if (subject_type == this->get_subject_type()) {
		return;
	}

	this->subject_type = subject_type;

	if (game::get()->is_running()) {
		emit subject_type_changed();
	}

	this->get_government()->check_government_type();
}

QVariantList country_game_data::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

void country_game_data::add_province(const province *province)
{
	this->explore_province(province);

	this->provinces.push_back(province);

	this->on_province_gained(province, 1);

	map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const resource *resource = tile->get_resource();

		if (resource != nullptr) {
			if (!tile->is_resource_discovered() && !resource->is_prospectable()) {
				assert_throw(resource->get_required_technology() != nullptr);

				if (this->get_technology()->has_technology(resource->get_required_technology())) {
					map::get()->set_tile_resource_discovered(tile_pos, true);
				}
			}
		}
	}

	if (province_game_data->is_country_border_province()) {
		this->border_provinces.push_back(province);
	}

	for (const metternich::province *neighbor_province : province_game_data->get_neighbor_provinces()) {
		const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->country) {
			continue;
		}

		//province ceased to be a country border province, remove it from the list
		if (vector::contains(this->get_border_provinces(), neighbor_province) && !neighbor_province_game_data->is_country_border_province()) {
			std::erase(this->border_provinces, neighbor_province);
		}

		for (const QPoint &tile_pos : neighbor_province_game_data->get_border_tiles()) {
			if (!map->is_tile_on_country_border(tile_pos)) {
				std::erase(this->border_tiles, tile_pos);
			}

			if (game::get()->is_running()) {
				map->calculate_tile_country_border_directions(tile_pos);
			}
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		if (map->is_tile_on_country_border(tile_pos)) {
			this->border_tiles.push_back(tile_pos);
		}
	}

	this->calculate_territory_rect();

	if (this->get_provinces().size() == 1) {
		game::get()->add_country(this->country);
	}

	for (const metternich::country *country : game::get()->get_countries()) {
		if (country == this->country) {
			continue;
		}

		if (!country->get_game_data()->is_country_known(this->country) && country->get_game_data()->is_province_discovered(province)) {
			country->get_game_data()->add_known_country(this->country);
		}
	}

	if (this->get_capital() == nullptr) {
		this->choose_capital();
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

void country_game_data::remove_province(const province *province)
{
	std::erase(this->provinces, province);

	if (this->get_capital_province() == province) {
		this->choose_capital();
	}

	this->on_province_gained(province, -1);

	map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		std::erase(this->border_tiles, tile_pos);
	}

	std::erase(this->border_provinces, province);

	for (const metternich::province *neighbor_province : province_game_data->get_neighbor_provinces()) {
		const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->country) {
			continue;
		}

		//province has become a country border province, add it to the list
		if (neighbor_province_game_data->is_country_border_province() && !vector::contains(this->get_border_provinces(), neighbor_province)) {
			this->border_provinces.push_back(neighbor_province);
		}

		for (const QPoint &tile_pos : neighbor_province_game_data->get_border_tiles()) {
			if (map->is_tile_on_country_border(tile_pos) && !vector::contains(this->get_border_tiles(), tile_pos)) {
				this->border_tiles.push_back(tile_pos);
			}

			if (game::get()->is_running()) {
				map->calculate_tile_country_border_directions(tile_pos);
			}
		}
	}

	this->calculate_territory_rect();

	//remove this as a known country for other countries, if they no longer have explored tiles in this country's territory
	for (const metternich::country *country : game::get()->get_countries()) {
		if (country == this->country) {
			continue;
		}

		if (!country->get_game_data()->is_country_known(this->country)) {
			continue;
		}

		if (!country->get_game_data()->is_province_discovered(province)) {
			//the other country didn't have this province discovered, so its removal from this country's territory couldn't have impacted its knowability to them anyway
			continue;
		}

		bool known_province = false;
		for (const metternich::province *loop_province : this->get_provinces()) {
			if (country->get_game_data()->is_province_discovered(loop_province)) {
				known_province = true;
				break;
			}
		}

		if (!known_province) {
			country->get_game_data()->remove_known_country(this->country);
		}
	}

	if (this->get_provinces().empty()) {
		game::get()->remove_country(this->country);
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

void country_game_data::on_province_gained(const province *province, const int multiplier)
{
	province_game_data *province_game_data = province->get_game_data();

	if (province_game_data->is_coastal()) {
		this->coastal_province_count += 1 * multiplier;
	}

	this->change_economic_score(province_game_data->get_level() * 100 * multiplier);

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->get_economy()->change_resource_count(resource, count * multiplier);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, count * multiplier);
		}
	}

	for (const auto &[terrain, count] : province_game_data->get_tile_terrain_counts()) {
		this->change_tile_terrain_count(terrain, count * multiplier);
	}

	for (const auto &[resource, commodity_map] : this->get_economy()->get_improved_resource_commodity_bonuses()) {
		for (const auto &[commodity, value] : commodity_map) {
			province_game_data->change_improved_resource_commodity_bonus(resource, commodity, value * multiplier);
		}
	}

	for (const auto &[commodity, threshold_map] : this->get_economy()->get_commodity_bonuses_for_tile_thresholds()) {
		for (const auto &[threshold, value] : threshold_map) {
			province_game_data->change_commodity_bonus_for_tile_threshold(commodity, threshold, value * multiplier);
		}
	}
}

void country_game_data::on_site_gained(const site *site, const int multiplier)
{
	const site_game_data *site_game_data = site->get_game_data();

	if (site->is_settlement() && site_game_data->is_built()) {
		this->change_settlement_count(1 * multiplier);

		for (const qunique_ptr<settlement_building_slot> &building_slot : site_game_data->get_building_slots()) {
			const building_type *building = building_slot->get_building();
			if (building != nullptr) {
				assert_throw(building->is_provincial());
				this->change_settlement_building_count(building, 1 * multiplier);
			}

			const wonder *wonder = building_slot->get_wonder();
			if (wonder != nullptr) {
				this->on_wonder_gained(wonder, multiplier);
			}
		}

		this->change_housing(site_game_data->get_housing() * multiplier);
	}

	const resource *site_resource = site->get_game_data()->get_resource();
	if (site_resource != nullptr) {
		if (site_resource->get_country_modifier() != nullptr) {
			site_resource->get_country_modifier()->apply(this->country, multiplier);
		}

		if (site_resource->get_improved_country_modifier() != nullptr && site->get_game_data()->get_resource_improvement() != nullptr) {
			site_resource->get_improved_country_modifier()->apply(this->country, multiplier);
		}
	}

	magic_enum::enum_for_each<improvement_slot>([this, site, multiplier](const improvement_slot slot) {
		const improvement *improvement = site->get_game_data()->get_improvement(slot);
		if (improvement != nullptr && improvement->get_country_modifier() != nullptr) {
			improvement->get_country_modifier()->apply(this->country, multiplier);
		}
	});
}

void country_game_data::set_capital(const site *capital)
{
	if (capital == this->get_capital()) {
		return;
	}

	if (capital != nullptr) {
		assert_throw(capital->is_settlement());
		assert_throw(capital->get_game_data()->get_province()->get_game_data()->get_owner() == this->country);
		assert_throw(capital->get_game_data()->is_built());
	}

	const site *old_capital = this->get_capital();

	this->capital = capital;

	if (capital != nullptr) {
		capital->get_game_data()->calculate_commodity_outputs();
		capital->get_game_data()->check_building_conditions();
	}

	if (old_capital != nullptr) {
		old_capital->get_game_data()->calculate_commodity_outputs();
		old_capital->get_game_data()->check_building_conditions();
	}

	emit capital_changed();
}

void country_game_data::choose_capital()
{
	if (this->country->get_default_capital()->get_game_data()->get_owner() == this->country && this->country->get_default_capital()->get_game_data()->can_be_capital()) {
		this->set_capital(this->country->get_default_capital());
		return;
	}

	const site *best_capital = nullptr;

	for (const metternich::province *province : this->get_provinces()) {
		for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
			const site_game_data *settlement_game_data = settlement->get_game_data();

			if (!settlement_game_data->can_be_capital()) {
				continue;
			}

			if (best_capital != nullptr) {
				if (best_capital->get_game_data()->is_provincial_capital() && !settlement_game_data->is_provincial_capital()) {
					continue;
				}

				if (best_capital->get_game_data()->get_settlement_type()->get_level() >= settlement_game_data->get_settlement_type()->get_level()) {
					continue;
				}
			}

			best_capital = settlement;
		}
	}

	this->set_capital(best_capital);
}

const province *country_game_data::get_capital_province() const
{
	if (this->get_capital() != nullptr) {
		return this->get_capital()->get_game_data()->get_province();
	}

	return nullptr;
}

void country_game_data::change_settlement_count(const int change)
{
	if (change == 0) {
		return;
	}

	const int old_settlement_count = this->get_settlement_count();

	this->settlement_count += change;

	for (const auto &[building, count] : this->settlement_building_counts) {
		if (building->get_weighted_country_modifier() != nullptr) {
			building->get_weighted_country_modifier()->apply(this->country, centesimal_int(-count) / old_settlement_count);
		}
	}

	if (this->get_settlement_count() != 0) {
		for (const auto &[building, count] : this->settlement_building_counts) {
			if (building->get_weighted_country_modifier() != nullptr) {
				//reapply the settlement building's weighted country modifier with the updated settlement count
				building->get_weighted_country_modifier()->apply(this->country, centesimal_int(count) / this->get_settlement_count());
			}
		}
	}
}

void country_game_data::calculate_territory_rect()
{
	QRect territory_rect;
	this->contiguous_territory_rects.clear();

	for (const province *province : this->get_provinces()) {
		const province_game_data *province_game_data = province->get_game_data();

		if (territory_rect.isNull()) {
			territory_rect = province_game_data->get_territory_rect();
		} else {
			territory_rect = territory_rect.united(province_game_data->get_territory_rect());
		}

		this->contiguous_territory_rects.push_back(province_game_data->get_territory_rect());
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (size_t i = 0; i < this->contiguous_territory_rects.size(); ++i) {
			QRect &first_territory_rect = this->contiguous_territory_rects.at(i);

			for (size_t j = i + 1; j < this->contiguous_territory_rects.size();) {
				const QRect &second_territory_rect = this->contiguous_territory_rects.at(j);

				if (first_territory_rect.intersects(second_territory_rect) || rect::is_adjacent_to(first_territory_rect, second_territory_rect)) {
					first_territory_rect = first_territory_rect.united(second_territory_rect);
					this->contiguous_territory_rects.erase(this->contiguous_territory_rects.begin() + j);
					changed = true;
				} else {
					++j;
				}
			}
		}
	}

	this->territory_rect = territory_rect;

	const QPoint &capital_pos = this->get_capital() ? this->get_capital()->get_game_data()->get_tile_pos() : this->country->get_default_capital()->get_game_data()->get_tile_pos();
	int best_distance = std::numeric_limits<int>::max();
	for (const QRect &contiguous_territory_rect : this->get_contiguous_territory_rects()) {
		if (contiguous_territory_rect.contains(capital_pos)) {
			this->main_contiguous_territory_rect = contiguous_territory_rect;
			break;
		}

		int distance = rect::distance_to(contiguous_territory_rect, capital_pos);

		if (distance < best_distance) {
			best_distance = distance;
			this->main_contiguous_territory_rect = contiguous_territory_rect;
		}
	}

	this->calculate_territory_rect_center();
	this->calculate_text_rect();

	if (game::get()->is_running()) {
		this->country->get_turn_data()->set_diplomatic_map_dirty(true);
	}
}

void country_game_data::calculate_territory_rect_center()
{
	if (this->get_provinces().empty()) {
		return;
	}

	QPoint sum(0, 0);
	int tile_count = 0;

	for (const province *province : this->get_provinces()) {
		const province_map_data *province_map_data = province->get_map_data();
		if (!this->get_main_contiguous_territory_rect().contains(province_map_data->get_territory_rect())) {
			continue;
		}

		const int province_tile_count = static_cast<int>(province_map_data->get_tiles().size());
		sum += province_map_data->get_territory_rect_center() * province_tile_count;
		tile_count += province_tile_count;
	}

	this->territory_rect_center = QPoint(sum.x() / tile_count, sum.y() / tile_count);
}

QVariantList country_game_data::get_contiguous_territory_rects_qvariant_list() const
{
	return container::to_qvariant_list(this->get_contiguous_territory_rects());
}

void country_game_data::calculate_text_rect()
{
	this->text_rect = QRect();

	if (!this->is_alive()) {
		return;
	}

	QPoint center_pos = this->get_territory_rect_center();

	const map *map = map::get();

	if (map->get_tile(center_pos)->get_owner() != this->country && this->get_capital() != nullptr) {
		center_pos = this->get_capital()->get_game_data()->get_tile_pos();

		if (map->get_tile(center_pos)->get_owner() != this->country) {
			return;
		}
	}

	this->text_rect = QRect(center_pos, QSize(1, 1));

	bool changed = true;
	while (changed) {
		changed = false;

		bool can_expand_left = true;
		const int left_x = this->text_rect.left() - 1;
		for (int y = this->text_rect.top(); y <= this->text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(left_x, y);

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_left = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
				can_expand_left = false;
				break;
			}
		}
		if (can_expand_left) {
			this->text_rect.setLeft(left_x);
			changed = true;
		}

		bool can_expand_right = true;
		const int right_x = this->text_rect.right() + 1;
		for (int y = this->text_rect.top(); y <= this->text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(right_x, y);

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_right = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
				can_expand_right = false;
				break;
			}
		}
		if (can_expand_right) {
			this->text_rect.setRight(right_x);
			changed = true;
		}

		bool can_expand_up = true;
		const int up_y = this->text_rect.top() - 1;
		for (int x = this->text_rect.left(); x <= this->text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, up_y);

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_up = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
				can_expand_up = false;
				break;
			}
		}
		if (can_expand_up) {
			this->text_rect.setTop(up_y);
			changed = true;
		}

		bool can_expand_down = true;
		const int down_y = this->text_rect.bottom() + 1;
		for (int x = this->text_rect.left(); x <= this->text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, down_y);

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_down = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
				can_expand_down = false;
				break;
			}
		}
		if (can_expand_down) {
			this->text_rect.setBottom(down_y);
			changed = true;
		}
	}
}

QVariantList country_game_data::get_tile_terrain_counts_qvariant_list() const
{
	QVariantList counts = archimedes::map::to_qvariant_list(this->get_tile_terrain_counts());
	std::sort(counts.begin(), counts.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toInt() > rhs.toMap().value("value").toInt();
	});
	return counts;
}

void country_game_data::add_known_country(const metternich::country *other_country)
{
	this->known_countries.insert(other_country);

	const consulate *current_consulate = this->get_consulate(other_country);

	const consulate *best_free_consulate = nullptr;
	for (const auto &[consulate, count] : this->free_consulate_counts) {
		if (best_free_consulate == nullptr || consulate->get_level() > best_free_consulate->get_level()) {
			best_free_consulate = consulate;
		}
	}

	if (best_free_consulate != nullptr && (current_consulate == nullptr || current_consulate->get_level() < best_free_consulate->get_level())) {
		this->set_consulate(other_country, best_free_consulate);
	}

	if (this->get_technology()->get_gain_technologies_known_by_others_count() > 0) {
		this->get_technology()->gain_technologies_known_by_others();
	}
}

diplomacy_state country_game_data::get_diplomacy_state(const metternich::country *other_country) const
{
	const auto find_iterator = this->diplomacy_states.find(other_country);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

void country_game_data::set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state)
{
	const diplomacy_state old_state = this->get_diplomacy_state(other_country);

	if (state == old_state) {
		return;
	}

	if (is_vassalage_diplomacy_state(state)) {
		this->set_overlord(other_country);
	} else {
		if (this->get_overlord() == other_country) {
			this->set_overlord(nullptr);
		}
	}

	if (old_state != diplomacy_state::peace) {
		this->change_diplomacy_state_count(old_state, -1);
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_country);
	} else {
		this->diplomacy_states[other_country] = state;
		this->change_diplomacy_state_count(state, 1);
	}

	if (game::get()->is_running()) {
		emit diplomacy_states_changed();

		if (is_vassalage_diplomacy_state(state) || is_vassalage_diplomacy_state(old_state)) {
			emit type_name_changed();
		}
	}
}

void country_game_data::change_diplomacy_state_count(const diplomacy_state state, const int change)
{
	const int final_count = (this->diplomacy_state_counts[state] += change);

	if (final_count == 0) {
		this->diplomacy_state_counts.erase(state);
		this->diplomacy_state_diplomatic_map_images.erase(state);
	}

	//if the change added the diplomacy state to the map, then we need to create the diplomatic map image for it
	if (game::get()->is_running() && final_count == change && !is_vassalage_diplomacy_state(state) && !is_overlordship_diplomacy_state(state)) {
		this->country->get_turn_data()->set_diplomatic_map_diplomacy_state_dirty(state);
	}
}

QString country_game_data::get_diplomacy_state_diplomatic_map_suffix(metternich::country *other_country) const
{
	if (other_country == this->country || this->is_any_overlord_of(other_country) || this->is_any_vassal_of(other_country)) {
		return "empire";
	}

	return QString::fromStdString(std::string(magic_enum::enum_name(this->get_diplomacy_state(other_country))));
}

bool country_game_data::at_war() const
{
	return this->diplomacy_state_counts.contains(diplomacy_state::war);
}

bool country_game_data::can_attack(const metternich::country *other_country) const
{
	if (other_country == nullptr) {
		return false;
	}

	if (other_country == this->country) {
		return false;
	}

	if (this->is_any_overlord_of(other_country)) {
		return false;
	}

	if (other_country->get_government()->is_clade()) {
		return true;
	} else if (this->get_government()->is_clade()) {
		return false;
	}

	switch (this->get_diplomacy_state(other_country)) {
		case diplomacy_state::non_aggression_pact:
		case diplomacy_state::alliance:
			return false;
		case diplomacy_state::war:
			return true;
		default:
			break;
	}

	if (other_country->get_government()->is_tribal() || this->get_government()->is_tribal()) {
		return true;
	}

	if (other_country->get_game_data()->is_under_anarchy() || this->is_under_anarchy()) {
		return true;
	}

	return false;
}

std::optional<diplomacy_state> country_game_data::get_offered_diplomacy_state(const metternich::country *other_country) const
{
	const auto find_iterator = this->offered_diplomacy_states.find(other_country);

	if (find_iterator != this->offered_diplomacy_states.end()) {
		return find_iterator->second;
	}

	return std::nullopt;
}

void country_game_data::set_offered_diplomacy_state(const metternich::country *other_country, const std::optional<diplomacy_state> &state)
{
	const diplomacy_state old_state = this->get_diplomacy_state(other_country);

	if (state == old_state) {
		return;
	}

	if (state.has_value()) {
		this->offered_diplomacy_states[other_country] = state.value();
	} else {
		this->offered_diplomacy_states.erase(other_country);
	}

	if (game::get()->is_running()) {
		emit offered_diplomacy_states_changed();
	}
}

QVariantList country_game_data::get_consulates_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->consulates);
}

void country_game_data::set_consulate(const metternich::country *other_country, const consulate *consulate)
{
	if (consulate == nullptr) {
		this->consulates.erase(other_country);
	} else {
		this->consulates[other_country] = consulate;

		if (other_country->get_game_data()->get_consulate(this->country) != consulate) {
			other_country->get_game_data()->set_consulate(this->country, consulate);
		}
	}

	if (game::get()->is_running()) {
		emit consulates_changed();
	}
}

int country_game_data::get_opinion_of(const metternich::country *other) const
{
	int opinion = this->get_base_opinion(other);

	for (const auto &[modifier, duration] : this->get_opinion_modifiers_for(other)) {
		opinion += modifier->get_value();
	}

	opinion = std::clamp(opinion, country::min_opinion, country::max_opinion);

	return opinion;
}

void country_game_data::set_base_opinion(const metternich::country *other, const int opinion)
{
	assert_throw(other != this->country);

	if (opinion == this->get_base_opinion(other)) {
		return;
	}

	if (opinion < country::min_opinion) {
		this->set_base_opinion(other, country::min_opinion);
		return;
	} else if (opinion > country::max_opinion) {
		this->set_base_opinion(other, country::max_opinion);
		return;
	}

	if (opinion == 0) {
		this->base_opinions.erase(other);
	} else {
		this->base_opinions[other] = opinion;
	}
}

void country_game_data::add_opinion_modifier(const metternich::country *other, const opinion_modifier *modifier, const int duration)
{
	this->opinion_modifiers[other][modifier] = std::max(this->opinion_modifiers[other][modifier], duration);
}

void country_game_data::remove_opinion_modifier(const metternich::country *other, const opinion_modifier *modifier)
{
	opinion_modifier_map<int> &opinion_modifiers = this->opinion_modifiers[other];
	opinion_modifiers.erase(modifier);

	if (opinion_modifiers.empty()) {
		this->opinion_modifiers.erase(other);
	}
}

int country_game_data::get_opinion_weighted_prestige_for(const metternich::country *other) const
{
	const int opinion = this->get_opinion_of(other);
	const int prestige = std::max(1, other->get_economy()->get_stored_commodity(defines::get()->get_prestige_commodity()));

	const int opinion_weighted_prestige = prestige * (opinion - country::min_opinion) / (country::max_opinion - country::min_opinion);
	return opinion_weighted_prestige;
}

std::vector<const metternich::country *> country_game_data::get_vassals() const
{
	std::vector<const metternich::country *> vassals;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			vassals.push_back(country);
		}
	}

	return vassals;
}

QVariantList country_game_data::get_vassals_qvariant_list() const
{
	return container::to_qvariant_list(this->get_vassals());
}

QVariantList country_game_data::get_subject_type_counts_qvariant_list() const
{
	std::map<const metternich::subject_type *, int> subject_type_counts;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			assert_throw(country->get_game_data()->get_subject_type() != nullptr);
			++subject_type_counts[country->get_game_data()->get_subject_type()];
		}
	}

	QVariantList counts = archimedes::map::to_qvariant_list(subject_type_counts);
	std::sort(counts.begin(), counts.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toInt() > rhs.toMap().value("value").toInt();
	});

	return counts;
}

std::vector<const metternich::country *> country_game_data::get_neighbor_countries() const
{
	std::vector<const metternich::country *> neighbor_countries;

	for (const province *province : this->get_border_provinces()) {
		for (const metternich::province *neighbor_province : province->get_game_data()->get_neighbor_provinces()) {
			const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
			if (neighbor_province_game_data->get_owner() == this->country) {
				continue;
			}

			if (neighbor_province_game_data->get_owner() == nullptr) {
				continue;
			}

			if (vector::contains(neighbor_countries, neighbor_province_game_data->get_owner())) {
				continue;
			}

			neighbor_countries.push_back(neighbor_province_game_data->get_owner());
		}
	}

	return neighbor_countries;
}

const QColor &country_game_data::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_diplomatic_map_color();
	}

	return this->country->get_color();
}

QImage country_game_data::prepare_diplomatic_map_image() const
{
	assert_throw(this->territory_rect.width() > 0);
	assert_throw(this->territory_rect.height() > 0);

	QImage image(this->territory_rect.size(), QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	return image;
}

QCoro::Task<QImage> country_game_data::finalize_diplomatic_map_image(QImage &&image) const
{
	QImage scaled_image;

	const int tile_pixel_size = map::get()->get_diplomatic_map_tile_pixel_size();

	co_await QtConcurrent::run([tile_pixel_size, &image, &scaled_image]() {
		scaled_image = image::scale<QImage::Format_ARGB32>(image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	image = std::move(scaled_image);

	std::vector<QPoint> border_pixels;

	for (int x = 0; x < image.width(); ++x) {
		for (int y = 0; y < image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (image.width() - 1) || pixel_pos.y() == (image.height() - 1)) {
				border_pixels.push_back(pixel_pos);
				continue;
			}

			if (pixel_color.alpha() != 255) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			bool is_border_pixel = false;
			point::for_each_cardinally_adjacent_until(pixel_pos, [&image, &is_border_pixel](const QPoint &adjacent_pos) {
				if (image.pixelColor(adjacent_pos).alpha() != 0) {
					return false;
				}

				is_border_pixel = true;
				return true;
			});

			if (is_border_pixel) {
				border_pixels.push_back(pixel_pos);
			}
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : border_pixels) {
		image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	co_await QtConcurrent::run([&scale_factor, &image, &scaled_image]() {
		scaled_image = image::scale<QImage::Format_ARGB32>(image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	co_return scaled_image;
}

QCoro::Task<void> country_game_data::create_diplomatic_map_image()
{
	const map *map = map::get();

	QImage diplomatic_map_image = this->prepare_diplomatic_map_image();
	QImage selected_diplomatic_map_image = diplomatic_map_image;

	const QColor &color = this->get_diplomatic_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	for (int x = 0; x < this->territory_rect.width(); ++x) {
		for (int y = 0; y < this->territory_rect.height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->territory_rect.topLeft() + relative_tile_pos);

			if (tile->get_owner() != this->country) {
				continue;
			}

			diplomatic_map_image.setPixelColor(relative_tile_pos, color);
			selected_diplomatic_map_image.setPixelColor(relative_tile_pos, selected_color);
		}
	}

	this->diplomatic_map_image = co_await this->finalize_diplomatic_map_image(std::move(diplomatic_map_image));
	this->selected_diplomatic_map_image = co_await this->finalize_diplomatic_map_image(std::move(selected_diplomatic_map_image));

	const int tile_pixel_size = map->get_diplomatic_map_tile_pixel_size();
	this->diplomatic_map_image_rect = QRect(this->territory_rect.topLeft() * tile_pixel_size * preferences::get()->get_scale_factor(), this->diplomatic_map_image.size());

	co_await this->create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic);
	co_await this->create_diplomacy_state_diplomatic_map_image(diplomacy_state::peace);

	for (const auto &[diplomacy_state, count] : this->get_diplomacy_state_counts()) {
		if (!is_vassalage_diplomacy_state(diplomacy_state) && !is_overlordship_diplomacy_state(diplomacy_state)) {
			co_await this->create_diplomacy_state_diplomatic_map_image(diplomacy_state);
		}
	}

	co_await this->create_diplomatic_map_mode_image(diplomatic_map_mode::terrain);
	co_await this->create_diplomatic_map_mode_image(diplomatic_map_mode::cultural);
	co_await this->create_diplomatic_map_mode_image(diplomatic_map_mode::religious);

	emit diplomatic_map_image_changed();
}

QCoro::Task<void> country_game_data::create_diplomatic_map_mode_image(const diplomatic_map_mode mode)
{
	static const QColor empty_color(Qt::black);
	static constexpr QColor diplomatic_self_color(170, 148, 214);

	const map *map = map::get();

	QImage image = this->prepare_diplomatic_map_image();

	for (int x = 0; x < this->territory_rect.width(); ++x) {
		for (int y = 0; y < this->territory_rect.height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->territory_rect.topLeft() + relative_tile_pos);

			if (tile->get_owner() != this->country) {
				continue;
			}

			const QColor *color = nullptr;

			switch (mode) {
				case diplomatic_map_mode::diplomatic:
					color = &diplomatic_self_color;
					break;
				case diplomatic_map_mode::terrain:
					color = &tile->get_terrain()->get_color();
					break;
				case diplomatic_map_mode::cultural: {
					const culture *culture = nullptr;

					if (tile->get_site() != nullptr && tile->get_site()->get_game_data()->can_have_population() && tile->get_site()->get_game_data()->get_culture() != nullptr) {
						culture = tile->get_site()->get_game_data()->get_culture();
					} else {
						culture = tile->get_province()->get_game_data()->get_culture();
					}

					if (culture != nullptr) {
						color = &culture->get_color();
					} else {
						color = &empty_color;
					}
					break;
				}
				case diplomatic_map_mode::religious: {
					const metternich::religion *religion = nullptr;

					if (tile->get_settlement() != nullptr && tile->get_settlement()->get_game_data()->get_religion() != nullptr) {
						religion = tile->get_settlement()->get_game_data()->get_religion();
					} else {
						religion = tile->get_province()->get_game_data()->get_religion();
					}

					if (religion != nullptr) {
						color = &religion->get_color();
					} else {
						color = &empty_color;
					}
					break;
				}
			}

			image.setPixelColor(relative_tile_pos, *color);
		}
	}

	this->diplomatic_map_mode_images[mode] = co_await this->finalize_diplomatic_map_image(std::move(image));
}

QCoro::Task<void> country_game_data::create_diplomacy_state_diplomatic_map_image(const diplomacy_state state)
{
	static const QColor empty_color(Qt::black);

	const map *map = map::get();

	QImage image = this->prepare_diplomatic_map_image();

	for (int x = 0; x < this->territory_rect.width(); ++x) {
		for (int y = 0; y < this->territory_rect.height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->territory_rect.topLeft() + relative_tile_pos);

			if (tile->get_owner() != this->country) {
				continue;
			}

			const QColor &color = defines::get()->get_diplomacy_state_color(state);

			image.setPixelColor(relative_tile_pos, color);
		}
	}

	this->diplomacy_state_diplomatic_map_images[state] = co_await this->finalize_diplomatic_map_image(std::move(image));
}

void country_game_data::change_score(const int change)
{
	if (change == 0) {
		return;
	}

	this->score += change;

	emit score_changed();
}

void country_game_data::change_economic_score(const int change)
{
	if (change == 0) {
		return;
	}

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(-this->get_economic_score() * country_game_data::vassal_tax_rate / 100);
	}

	this->economic_score += change;

	this->change_score(change);

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(this->get_economic_score() * country_game_data::vassal_tax_rate / 100);
	}
}

void country_game_data::change_military_score(const int change)
{
	if (change == 0) {
		return;
	}

	this->military_score += change;

	this->change_score(change);
}

const population_class *country_game_data::get_default_population_class() const
{
	if (this->get_government()->is_tribal() || this->get_government()->is_clade()) {
		return defines::get()->get_default_tribal_population_class();
	} else {
		return defines::get()->get_default_population_class();
	}
}

void country_game_data::add_population_unit(population_unit *population_unit)
{
	this->population_units.push_back(population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void country_game_data::remove_population_unit(population_unit *population_unit)
{
	std::erase(this->population_units, population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void country_game_data::on_population_type_count_changed(const population_type *type, const int change)
{
	//countries generate demand in the world market depending on population commodity demand
	for (const auto &[commodity, value] : type->get_commodity_demands()) {
		this->get_economy()->change_commodity_demand(commodity, value * change);
	}

	this->change_food_consumption(change);

	if (type->get_country_modifier() != nullptr) {
		const int population_type_count = this->get_population()->get_type_count(type);
		const int old_population_type_count = population_type_count - change;
		const centesimal_int &type_modifier_multiplier = this->get_population_type_modifier_multiplier(type);
		const centesimal_int &max_total_modifier_multiplier = type->get_max_modifier_multiplier();

		type->get_country_modifier()->apply(this->country, -centesimal_int::min(old_population_type_count * type_modifier_multiplier, max_total_modifier_multiplier));
		type->get_country_modifier()->apply(this->country, centesimal_int::min(population_type_count * type_modifier_multiplier, max_total_modifier_multiplier));
	}
}

std::vector<const phenotype *> country_game_data::get_weighted_phenotypes() const
{
	std::vector<const phenotype *> weighted_phenotypes = this->get_population()->get_weighted_phenotypes_for_culture(this->country->get_culture());

	if (weighted_phenotypes.empty()) {
		weighted_phenotypes = this->country->get_culture()->get_weighted_phenotypes();
	}

	return weighted_phenotypes;
}

void country_game_data::set_population_growth(const int growth)
{
	if (growth == this->get_population_growth()) {
		return;
	}

	const int change = growth - this->get_population_growth();

	this->population_growth = growth;

	this->get_population()->change_size(change * defines::get()->get_population_per_unit() / defines::get()->get_population_growth_threshold());

	if (game::get()->is_running()) {
		emit population_growth_changed();
	}
}

void country_game_data::grow_population()
{
	if (this->population_units.empty()) {
		throw std::runtime_error("Tried to grow population in a country which has no pre-existing population.");
	}

	std::vector<population_unit *> potential_base_population_units = this->population_units;

	std::erase_if(potential_base_population_units, [this](const population_unit *population_unit) {
		if (population_unit->get_site()->get_game_data()->get_available_housing() <= 0) {
			return true;
		}

		return false;
	});

	if (potential_base_population_units.empty()) {
		//this could happen if the settlements with available housing have no population
		potential_base_population_units = this->population_units;
	}

	assert_throw(!potential_base_population_units.empty());

	const population_unit *population_unit = vector::get_random(potential_base_population_units);
	const metternich::culture *culture = population_unit->get_culture();
	const metternich::religion *religion = population_unit->get_religion();
	const phenotype *phenotype = population_unit->get_phenotype();

	const site *site = population_unit->get_site();
	if (site->get_game_data()->get_available_housing() <= 0) {
		//if the population unit's site has no available housing, but there are empty populatable sites, grow the population in one of them
		std::vector<const metternich::site *> potential_sites;
		for (const province *province : this->get_provinces()) {
			for (const metternich::site *province_site : province->get_game_data()->get_sites()) {
				if (!province_site->get_game_data()->can_have_population() || !province_site->get_game_data()->is_built()) {
					continue;
				}

				if (province_site->get_game_data()->get_available_housing() <= 0) {
					continue;
				}

				potential_sites.push_back(province_site);
			}
		}

		assert_throw(!potential_sites.empty());

		site = vector::get_random(potential_sites);
	}

	const population_type *population_type = culture->get_population_class_type(site->get_game_data()->get_default_population_class());

	site->get_game_data()->create_population_unit(population_type, culture, religion, phenotype);

	this->change_population_growth(-defines::get()->get_population_growth_threshold());
}

void country_game_data::decrease_population(const bool change_population_growth)
{
	//disband population unit, if possible
	if (!this->population_units.empty()) {
		population_unit *population_unit = this->choose_starvation_population_unit();
		if (population_unit != nullptr) {
			if (change_population_growth) {
				this->change_population_growth(1);
			}
			population_unit->get_province()->get_game_data()->remove_population_unit(population_unit);
			population_unit->get_site()->get_game_data()->pop_population_unit(population_unit);
			return;
		}
	}

	assert_throw(false);
}

population_unit *country_game_data::choose_starvation_population_unit()
{
	std::vector<population_unit *> population_units;

	bool found_non_food_producer = false;
	bool found_producer = false;
	bool found_labor_producer = false;
	int lowest_output_value = std::numeric_limits<int>::max();
	for (population_unit *population_unit : this->get_population_units()) {
		if (population_unit->get_site()->is_settlement() && population_unit->get_site()->get_game_data()->get_population_unit_count() == 1) {
			//do not remove a settlement's last population unit
			continue;
		}

		const bool is_non_food_producer = !population_unit->is_food_producer();
		if (found_non_food_producer && !is_non_food_producer) {
			continue;
		} else if (!found_non_food_producer && is_non_food_producer) {
			found_non_food_producer = true;
			population_units.clear();
		}

		const bool is_producer = population_unit->get_type()->get_output_commodity() != nullptr;
		if (found_producer && !is_producer) {
			continue;
		} else if (!found_producer && is_producer) {
			found_producer = true;
			lowest_output_value = population_unit->get_type()->get_output_value();
			population_units.clear();
		}

		const bool is_labor_producer = population_unit->get_type()->get_output_commodity() != nullptr && population_unit->get_type()->get_output_commodity()->is_labor();
		if (found_labor_producer && !is_labor_producer) {
			continue;
		} else if (!found_labor_producer && is_labor_producer) {
			found_labor_producer = true;
			lowest_output_value = population_unit->get_type()->get_output_value();
			population_units.clear();
		}

		if (population_unit->get_type()->get_output_commodity() != nullptr) {
			if (population_unit->get_type()->get_output_value() > lowest_output_value) {
				continue;
			} else if (population_unit->get_type()->get_output_value() < lowest_output_value) {
				lowest_output_value = population_unit->get_type()->get_output_value();
				population_units.clear();
			}
		}

		population_units.push_back(population_unit);
	}

	if (population_units.empty()) {
		return nullptr;
	}

	return vector::get_random(population_units);
}

const icon *country_game_data::get_population_type_small_icon(const population_type *type) const
{
	icon_map<int> icon_counts;

	for (const auto &population_unit : this->population_units) {
		if (population_unit->get_type() != type) {
			continue;
		}

		++icon_counts[population_unit->get_small_icon()];
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	return best_icon;
}

int country_game_data::get_net_food_consumption() const
{
	int food_consumption = this->get_food_consumption();

	for (const province *province : this->get_provinces()) {
		for (const site *site : province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population()) {
				continue;
			}

			if (!site->get_game_data()->is_built()) {
				continue;
			}

			food_consumption -= site->get_game_data()->get_free_food_consumption();
		}
	}

	return food_consumption;
}

int country_game_data::get_available_food() const
{
	return this->get_economy()->get_stored_food() - this->get_net_food_consumption();
}

bool country_game_data::has_building(const building_type *building) const
{
	return this->get_settlement_building_count(building) > 0;
}

bool country_game_data::has_building_or_better(const building_type *building) const
{
	if (this->has_building(building)) {
		return true;
	}

	for (const building_type *requiring_building : building->get_requiring_buildings()) {
		if (this->has_building_or_better(requiring_building)) {
			return true;
		}
	}

	return false;
}

void country_game_data::change_settlement_building_count(const building_type *building, const int change)
{
	if (change == 0) {
		return;
	}

	const int old_count = this->get_settlement_building_count(building);

	const int count = (this->settlement_building_counts[building] += change);

	if (count < 0) {
		throw std::runtime_error(std::format("The settlement building count for country \"{}\" for building \"{}\" is negative ({}).", this->country->get_identifier(), building->get_identifier(), change));
	}

	if (count == 0) {
		this->settlement_building_counts.erase(building);
	}

	if (building->get_weighted_country_modifier() != nullptr && this->get_settlement_count() != 0) {
		//reapply the settlement building's weighted country modifier with the updated count
		building->get_weighted_country_modifier()->apply(this->country, centesimal_int(-old_count) / this->get_settlement_count());
		building->get_weighted_country_modifier()->apply(this->country, centesimal_int(count) / this->get_settlement_count());
	}

	if (building->get_country_modifier() != nullptr) {
		building->get_country_modifier()->apply(this->country, change);
	}

	if (game::get()->is_running()) {
		emit settlement_building_counts_changed();
	}
}

void country_game_data::on_wonder_gained(const wonder *wonder, const int multiplier)
{
	if (wonder->get_country_modifier() != nullptr) {
		wonder->get_country_modifier()->apply(this->country, multiplier);
	}

	if (multiplier > 0) {
		game::get()->set_wonder_country(wonder, this->country);
	} else if (multiplier < 0 && game::get()->get_wonder_country(wonder) == this->country) {
		game::get()->set_wonder_country(wonder, nullptr);
	}
}

bool country_game_data::can_declare_war_on(const metternich::country *other_country) const
{
	if (!this->country->can_declare_war()) {
		return false;
	}

	if (this->get_overlord() != nullptr) {
		return other_country == this->get_overlord();
	}

	return true;
}

QVariantList country_game_data::get_ideas_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_ideas());
}

const idea *country_game_data::get_idea(const idea_slot *slot) const
{
	const data_entry_map<idea_slot, const idea *> &ideas = this->get_ideas(slot->get_idea_type());
	const auto find_iterator = ideas.find(slot);

	if (find_iterator != ideas.end()) {
		return find_iterator->second;
	}

	return nullptr;
}

void country_game_data::set_idea(const idea_slot *slot, const idea *idea)
{
	const metternich::idea *old_idea = this->get_idea(slot);

	if (idea == old_idea) {
		return;
	}

	if (old_idea != nullptr) {
		old_idea->apply_modifier(this->country, -1);
	}

	if (idea != nullptr) {
		this->ideas[slot->get_idea_type()][slot] = idea;
	} else {
		this->ideas[slot->get_idea_type()].erase(slot);
		if (this->ideas[slot->get_idea_type()].empty()) {
			this->ideas.erase(slot->get_idea_type());
		}
	}

	if (idea != nullptr) {
		idea->apply_modifier(this->country, 1);
	}

	if (game::get()->is_running()) {
		emit ideas_changed();

		if (this->country == game::get()->get_player_country() && idea != nullptr) {
			const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

			switch (slot->get_idea_type()) {
				case idea_type::deity:
					engine_interface::get()->add_notification(std::format("{} Worshiped", idea->get_cultural_name(this->country->get_culture())), interior_minister_portrait, std::format("The cult of {} has become widespread in our nation!\n\n{}", idea->get_cultural_name(this->country->get_culture()), idea->get_modifier_string(this->country)));
					break;
				case idea_type::research_organization:
					engine_interface::get()->add_notification(std::format("{} Gains Contract", idea->get_name()), interior_minister_portrait, std::format("The {} has gained an important contract with our govenment to conduct research!\n\n{}", idea->get_name(), idea->get_modifier_string(this->country)));
					break;
				default:
					assert_throw(false);
			}
		}
	}
}

QVariantList country_game_data::get_appointed_ideas_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_appointed_ideas());
}

const idea *country_game_data::get_appointed_idea(const idea_slot *slot) const
{
	const data_entry_map<idea_slot, const idea *> &appointed_ideas = this->get_appointed_ideas(slot->get_idea_type());
	const auto find_iterator = appointed_ideas.find(slot);

	if (find_iterator != appointed_ideas.end()) {
		return find_iterator->second;
	}

	return nullptr;
}

void country_game_data::set_appointed_idea(const idea_slot *slot, const idea *idea)
{
	const metternich::idea *old_idea = this->get_appointed_idea(slot);

	if (idea == old_idea) {
		return;
	}

	if (idea != nullptr) {
		const commodity_map<int> commodity_costs = this->get_idea_commodity_costs(idea);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->get_economy()->change_stored_commodity(commodity, -cost);
		}

		this->appointed_ideas[slot->get_idea_type()][slot] = idea;
	} else {
		this->appointed_ideas[slot->get_idea_type()].erase(slot);
		if (this->appointed_ideas[slot->get_idea_type()].empty()) {
			this->appointed_ideas.erase(slot->get_idea_type());
		}
	}

	if (old_idea != nullptr) {
		const commodity_map<int> commodity_costs = this->get_idea_commodity_costs(old_idea);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->get_economy()->change_stored_commodity(commodity, cost);
		}
	}

	if (game::get()->is_running()) {
		emit appointed_ideas_changed();
	}
}

void country_game_data::check_idea(const idea_slot *slot)
{
	if (this->is_under_anarchy()) {
		this->set_idea(slot, nullptr);
		return;
	}

	//process appointment, if any
	const idea *appointed_idea = this->get_appointed_idea(slot);
	if (appointed_idea != nullptr && this->can_have_idea(slot, appointed_idea)) {
		this->set_idea(slot, appointed_idea);
	}

	//remove research organizations if they have become obsolete
	const idea *old_idea = this->get_idea(slot);
	if (old_idea != nullptr && old_idea->get_obsolescence_technology() != nullptr && this->get_technology()->has_technology(old_idea->get_obsolescence_technology())) {
		this->set_idea(slot, nullptr);

		if (game::get()->is_running()) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

				switch (slot->get_idea_type()) {
					case idea_type::deity:
						engine_interface::get()->add_notification(std::format("{} No Longer Worshiped", old_idea->get_cultural_name(this->country->get_culture())), interior_minister_portrait, std::format("Your Excellency, despite a long and proud history of being worshiped in our nation, the cult of {} has lost favor amongst our people, and declined to nothingness.", old_idea->get_cultural_name(this->country->get_culture())));
						break;
					case idea_type::research_organization:
						engine_interface::get()->add_notification(std::format("{} Abolished", old_idea->get_name()), interior_minister_portrait, std::format("Your Excellency, despite a long and proud history in conducting research for our nation, the {} has lost its ability to function, and has been abolished.", old_idea->get_name()));
						break;
					default:
						assert_throw(false);
				}
			}
		}
	}
}

void country_game_data::check_ideas()
{
	magic_enum::enum_for_each<idea_type>([this](const idea_type idea_type) {
		const std::vector<const idea_slot *> idea_slots = this->get_available_idea_slots(idea_type);
		for (const idea_slot *idea_slot : idea_slots) {
			this->check_idea(idea_slot);
		}

		const data_entry_map<idea_slot, const idea *> ideas = this->get_ideas(idea_type);
		for (const auto &[slot, slot_idea] : ideas) {
			if (!vector::contains(idea_slots, slot)) {
				this->set_idea(slot, nullptr);
			}
		}
	});

	this->appointed_ideas.clear();
}

std::vector<const idea *> country_game_data::get_appointable_ideas(const idea_slot *slot) const
{
	std::vector<const idea *> potential_ideas;

	switch (slot->get_idea_type()) {
		case idea_type::deity:
			for (const deity *deity : deity::get_all()) {
				potential_ideas.push_back(deity);
			}
			break;
		case idea_type::research_organization:
			for (const research_organization *research_organization : research_organization::get_all()) {
				potential_ideas.push_back(research_organization);
			}
			break;
		default:
			assert_throw(false);
	}

	std::erase_if(potential_ideas, [this, slot](const idea *idea) {
		if (!this->can_gain_idea(slot, idea)) {
			return true;
		}

		return false;
	});

	return potential_ideas;
}

QVariantList country_game_data::get_appointable_ideas_qvariant_list(const idea_slot *slot) const
{
	return container::to_qvariant_list(this->get_appointable_ideas(slot));
}

const idea *country_game_data::get_best_idea(const idea_slot *slot)
{
	std::vector<const idea *> potential_ideas;
	int best_skill = 0;

	for (const idea *idea : this->get_appointable_ideas(slot)) {
		if (!this->can_appoint_idea(slot, idea)) {
			continue;
		}

		const int skill = idea->get_skill();

		if (skill < best_skill) {
			continue;
		}

		if (skill > best_skill) {
			best_skill = skill;
			potential_ideas.clear();
		}

		potential_ideas.push_back(idea);
	}

	if (!potential_ideas.empty()) {
		const idea *idea = vector::get_random(potential_ideas);
		return idea;
	}

	return nullptr;
}

bool country_game_data::can_have_idea(const idea_slot *slot, const idea *idea) const
{
	if (!idea->is_available_for_country_slot(this->country, slot)) {
		return false;
	}

	for (const auto &[loop_slot, slot_idea] : this->get_ideas(slot->get_idea_type())) {
		if (slot_idea == idea) {
			return false;
		}
	}

	return true;
}

bool country_game_data::can_gain_idea(const idea_slot *slot, const idea *idea) const
{
	if (!this->can_have_idea(slot, idea)) {
		return false;
	}

	for (const auto &[loop_slot, slot_idea] : this->get_appointed_ideas(slot->get_idea_type())) {
		if (slot_idea == idea) {
			return false;
		}
	}

	return true;
}

bool country_game_data::can_appoint_idea(const idea_slot *slot, const idea *idea) const
{
	if (!this->can_gain_idea(slot, idea)) {
		return false;
	}

	const commodity_map<int> commodity_costs = this->get_idea_commodity_costs(idea);
	for (const auto &[commodity, cost] : commodity_costs) {
		if (this->get_economy()->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	return true;
}

std::vector<const idea_slot *> country_game_data::get_available_idea_slots(const idea_type idea_type) const
{
	std::vector<const idea_slot *> available_idea_slots;

	switch (idea_type) {
		case idea_type::deity:
			for (const deity_slot *slot : deity_slot::get_all()) {
				available_idea_slots.push_back(slot);
			}
			break;
		case idea_type::research_organization:
			for (const research_organization_slot *slot : research_organization_slot::get_all()) {
				available_idea_slots.push_back(slot);
			}
			break;
		default:
			assert_throw(false);
	}

	std::erase_if(available_idea_slots, [this](const idea_slot *slot) {
		if (slot->get_conditions() != nullptr && !slot->get_conditions()->check(this->country, read_only_context(this->country))) {
			return true;
		}

		return false;
	});

	return available_idea_slots;
}

QVariantList country_game_data::get_available_deity_slots_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_idea_slots(idea_type::deity));
}

int country_game_data::get_deity_cost() const
{
	const data_entry_map<idea_slot, const idea *> &deities = this->get_ideas(idea_type::deity);
	const data_entry_map<idea_slot, const idea *> &appointed_deities = this->get_appointed_ideas(idea_type::deity);
	const int deity_count = static_cast<int>(deities.size() + appointed_deities.size());

	if (deity_count == 0) {
		return country_game_data::first_deity_cost;
	}

	return country_game_data::base_deity_cost + country_game_data::deity_cost_increment * (deity_count - 1) * deity_count / 2;
}

commodity_map<int> country_game_data::get_idea_commodity_costs(const idea *idea) const
{
	commodity_map<int> commodity_costs;

	if (idea->get_idea_type() == idea_type::deity) {
		const int cost = this->get_deity_cost();
		commodity_costs[defines::get()->get_piety_commodity()] = cost;
	}

	return commodity_costs;
}

QVariantList country_game_data::get_idea_commodity_costs_qvariant_list(const idea *idea) const
{
	return archimedes::map::to_qvariant_list(this->get_idea_commodity_costs(idea));
}

QVariantList country_game_data::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

bool country_game_data::has_scripted_modifier(const scripted_country_modifier *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

void country_game_data::add_scripted_modifier(const scripted_country_modifier *modifier, const int duration)
{
	const read_only_context ctx(this->country);

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (modifier->get_modifier() != nullptr) {
		this->apply_modifier(modifier->get_modifier(), 1);
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void country_game_data::remove_scripted_modifier(const scripted_country_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		this->apply_modifier(modifier->get_modifier(), -1);
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void country_game_data::decrement_scripted_modifiers()
{
	std::vector<const scripted_country_modifier *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_country_modifier *modifier : modifiers_to_remove) {
		this->remove_scripted_modifier(modifier);
	}

	//decrement opinion modifiers
	country_map<std::vector<const opinion_modifier *>> opinion_modifiers_to_remove;

	for (auto &[country, opinion_modifier_map] : this->opinion_modifiers) {
		for (auto &[modifier, duration] : opinion_modifier_map) {
			if (duration == -1) {
				//eternal
				continue;
			}

			--duration;

			if (duration == 0) {
				opinion_modifiers_to_remove[country].push_back(modifier);
			}
		}
	}

	for (const auto &[country, opinion_modifiers] : opinion_modifiers_to_remove) {
		for (const opinion_modifier *modifier : opinion_modifiers) {
			this->remove_opinion_modifier(country, modifier);
		}
	}
}

void country_game_data::apply_modifier(const modifier<const metternich::country> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	modifier->apply(this->country, multiplier);
}

std::vector<const character *> country_game_data::get_characters() const
{
	std::vector<const character *> characters;

	for (const auto &[office, office_holder] : this->get_government()->get_office_holders()) {
		characters.push_back(office_holder);
	}

	vector::merge(characters, this->get_military()->get_leaders());
	vector::merge(characters, this->get_civilian_characters());

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->get_governor() != nullptr) {
			characters.push_back(province->get_game_data()->get_governor());
		}
	}

	return characters;
}

void country_game_data::check_characters()
{
	const std::vector<const character *> characters = this->get_characters();
	const QDate &current_date = game::get()->get_next_date();
	for (const character *character : characters) {
		if (character->is_immortal()) {
			continue;
		}

		assert_throw(character->get_death_date().isValid());

		if (character->get_death_date() > current_date) {
			continue;
		}

		character->get_game_data()->die();
	}

	this->get_government()->check_office_holders();

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->check_governor();
	}
}

void country_game_data::generate_ruler()
{
	const government_type *government_type = this->get_government()->get_government_type();
	assert_throw(government_type != nullptr);

	std::vector<const species *> species_list = this->country->get_culture()->get_species();
	assert_throw(!species_list.empty());

	const species *species = nullptr;
	std::vector<const character_class *> potential_classes;

	while (potential_classes.empty() && !species_list.empty()) {
		species = vector::take_random(species_list);

		for (const character_class *character_class : government_type->get_ruler_character_classes()) {
			if (!character_class->is_allowed_for_species(species)) {
				continue;
			}

			potential_classes.push_back(character_class);
		}
	}

	assert_throw(!potential_classes.empty());

	const character_class *character_class = vector::get_random(potential_classes);

	const character *ruler = character::generate(species, character_class, 1, this->country->get_culture(), this->get_religion(), this->get_capital());
	this->get_government()->set_office_holder(defines::get()->get_ruler_office(), ruler);
}

bool country_game_data::has_civilian_character(const character *character) const
{
	for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
		if (civilian_unit->get_character() == character) {
			return true;
		}
	}

	return false;
}

std::vector<const character *> country_game_data::get_civilian_characters() const
{
	std::vector<const character *> civilian_characters;

	for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
		if (civilian_unit->get_character() != nullptr) {
			civilian_characters.push_back(civilian_unit->get_character());
		}
	}

	return civilian_characters;
}

void country_game_data::on_civilian_character_died(const character *character)
{
	if (this->country == game::get()->get_player_country()) {
		const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

		const std::string &civilian_unit_type_name = character->get_civilian_unit_type()->get_name();

		engine_interface::get()->add_notification(std::format("{} Retired", civilian_unit_type_name), interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the {} {} has decided to retire.", string::lowered(civilian_unit_type_name), character->get_full_name()));
	}

	assert_throw(character->get_game_data()->get_civilian_unit() != nullptr);
	character->get_game_data()->get_civilian_unit()->disband(true);
}

bool country_game_data::create_civilian_unit(const civilian_unit_type *civilian_unit_type, const site *deployment_site, const phenotype *phenotype)
{
	if (this->is_under_anarchy()) {
		return false;
	}

	if (deployment_site == nullptr) {
		deployment_site = this->get_capital();
	}

	assert_throw(deployment_site != nullptr);

	QPoint tile_pos = deployment_site->get_game_data()->get_tile_pos();

	if (map::get()->get_tile(tile_pos)->get_civilian_unit() != nullptr) {
		//tile already occupied
		const std::optional<QPoint> nearest_tile_pos = map::get()->get_nearest_available_tile_pos_for_civilian_unit(tile_pos);
		if (!nearest_tile_pos.has_value()) {
			return false;
		}

		tile_pos = nearest_tile_pos.value();
	}

	std::vector<const character *> potential_characters;

	for (const character *character : character::get_all()) {
		if (!character->has_role(character_role::civilian)) {
			continue;
		}

		if (character->get_civilian_unit_type() != civilian_unit_type) {
			continue;
		}

		if (character->get_game_data()->get_country() != nullptr && character->get_game_data()->get_country() != this->country) {
			continue;
		}

		if (character->get_game_data()->get_civilian_unit() != nullptr) {
			continue;
		}

		if (character->has_role(character_role::advisor) && character->get_game_data()->get_country() != this->country) {
			//if the character is an advisor, they must already have been recruited by the country as an advisor before being usable as a civilian unit
			continue;
		}

		if (phenotype != nullptr && character->get_phenotype() != phenotype) {
			continue;
		}

		if (character->get_game_data()->is_dead()) {
			continue;
		}

		if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->country, read_only_context(this->country))) {
			continue;
		}

		potential_characters.push_back(character);
	}

	qunique_ptr<civilian_unit> civilian_unit;

	if (!potential_characters.empty()) {
		const character *character = vector::get_random(potential_characters);
		civilian_unit = make_qunique<metternich::civilian_unit>(character, this->country);
	} else {
		if (phenotype == nullptr) {
			const std::vector<const metternich::phenotype *> weighted_phenotypes = this->get_weighted_phenotypes();
			if (weighted_phenotypes.empty()) {
				return false;
			}

			phenotype = vector::get_random(weighted_phenotypes);
		}

		assert_throw(phenotype != nullptr);

		civilian_unit = make_qunique<metternich::civilian_unit>(civilian_unit_type, this->country, phenotype);
	}

	assert_throw(civilian_unit != nullptr);

	civilian_unit->set_tile_pos(tile_pos);

	this->add_civilian_unit(std::move(civilian_unit));

	return true;
}

void country_game_data::add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit)
{
	if (civilian_unit->get_character() != nullptr && !civilian_unit->get_character()->has_role(character_role::advisor)) {
		civilian_unit->get_character()->get_game_data()->set_country(this->country);
	}

	this->add_unit_name(civilian_unit->get_name());
	this->civilian_units.push_back(std::move(civilian_unit));
}

void country_game_data::remove_civilian_unit(civilian_unit *civilian_unit)
{
	assert_throw(civilian_unit != nullptr);

	if (civilian_unit->get_character() != nullptr && !civilian_unit->get_character()->has_role(character_role::advisor)) {
		assert_throw(civilian_unit->get_character()->get_game_data()->get_country() == this->country);
		civilian_unit->get_character()->get_game_data()->set_country(nullptr);
	}

	this->remove_unit_name(civilian_unit->get_name());

	for (size_t i = 0; i < this->civilian_units.size(); ++i) {
		if (this->civilian_units[i].get() == civilian_unit) {
			this->civilian_units.erase(this->civilian_units.begin() + i);
			return;
		}
	}
}

bool country_game_data::can_gain_civilian_unit(const civilian_unit_type *civilian_unit_type) const
{
	if (civilian_unit_type->get_required_technology() != nullptr && !this->get_technology()->has_technology(civilian_unit_type->get_required_technology())) {
		return false;
	}

	if (this->country->get_culture()->get_civilian_class_unit_type(civilian_unit_type->get_unit_class()) != civilian_unit_type) {
		return false;
	}

	//FIXME: check whether the country has a building capable of training the civilian unit type

	return true;
}

void country_game_data::change_civilian_unit_recruitment_count(const civilian_unit_type *civilian_unit_type, const int change, const bool change_input_storage)
{
	if (change == 0) {
		return;
	}

	const int count = (this->civilian_unit_recruitment_counts[civilian_unit_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->civilian_unit_recruitment_counts.erase(civilian_unit_type);
	}

	if (change_input_storage) {
		const commodity_map<int> &commodity_costs = civilian_unit_type->get_commodity_costs();
		for (const auto &[commodity, cost] : commodity_costs) {
			assert_throw(commodity->is_storable());

			const int cost_change = cost * change;

			this->get_economy()->change_stored_commodity(commodity, -cost_change);
		}

		if (civilian_unit_type->get_wealth_cost() > 0) {
			const int wealth_cost_change = civilian_unit_type->get_wealth_cost() * change;
			this->get_economy()->change_wealth(-wealth_cost_change);
		}
	}
}

bool country_game_data::can_increase_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type) const
{
	if (!this->can_gain_civilian_unit(civilian_unit_type)) {
		return false;
	}

	for (const auto &[commodity, cost] : civilian_unit_type->get_commodity_costs()) {
		assert_throw(commodity->is_storable());
		if (this->get_economy()->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	if (civilian_unit_type->get_wealth_cost() != 0 && this->get_economy()->get_wealth() < civilian_unit_type->get_wealth_cost()) {
		return false;
	}

	return true;
}

void country_game_data::increase_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type)
{
	try {
		assert_throw(this->can_increase_civilian_unit_recruitment(civilian_unit_type));

		this->change_civilian_unit_recruitment_count(civilian_unit_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error increasing recruitment of the \"{}\" civilian unit type for country \"{}\".", civilian_unit_type->get_identifier(), this->country->get_identifier())));
	}
}

bool country_game_data::can_decrease_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type) const
{
	if (this->get_civilian_unit_recruitment_count(civilian_unit_type) == 0) {
		return false;
	}

	return true;
}

void country_game_data::decrease_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_civilian_unit_recruitment(civilian_unit_type));

		this->change_civilian_unit_recruitment_count(civilian_unit_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error decreasing recruitment of the \"{}\" civilian unit type for country \"{}\".", civilian_unit_type->get_identifier(), this->country->get_identifier())));
	}
}

QVariantList country_game_data::get_transporters_qvariant_list() const
{
	return container::to_qvariant_list(this->transporters);
}

bool country_game_data::create_transporter(const transporter_type *transporter_type, const phenotype *phenotype)
{
	assert_throw(transporter_type != nullptr);

	if (this->is_under_anarchy()) {
		return false;
	}

	qunique_ptr<transporter> transporter;

	if (phenotype == nullptr) {
		const std::vector<const metternich::phenotype *> weighted_phenotypes = this->get_weighted_phenotypes();
		assert_throw(!weighted_phenotypes.empty());
		phenotype = vector::get_random(weighted_phenotypes);
	}
	assert_throw(phenotype != nullptr);

	transporter = make_qunique<metternich::transporter>(transporter_type, this->country, phenotype);

	assert_throw(transporter != nullptr);

	this->add_transporter(std::move(transporter));

	return true;
}

void country_game_data::add_transporter(qunique_ptr<transporter> &&transporter)
{
	this->add_unit_name(transporter->get_name());
	this->transporters.push_back(std::move(transporter));

	emit transporters_changed();
}

void country_game_data::remove_transporter(transporter *transporter)
{
	this->remove_unit_name(transporter->get_name());

	for (size_t i = 0; i < this->transporters.size(); ++i) {
		if (this->transporters[i].get() == transporter) {
			this->transporters.erase(this->transporters.begin() + i);
			return;
		}
	}

	emit transporters_changed();
}

void country_game_data::change_transporter_recruitment_count(const transporter_type *transporter_type, const int change, const bool change_input_storage)
{
	if (change == 0) {
		return;
	}

	const int count = (this->transporter_recruitment_counts[transporter_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->transporter_recruitment_counts.erase(transporter_type);
	}

	if (change_input_storage) {
		const int old_count = count - change;
		const commodity_map<int> old_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, old_count);
		const commodity_map<int> new_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, count);

		for (const auto &[commodity, cost] : new_commodity_costs) {
			assert_throw(commodity->is_storable());

			const int cost_change = cost - old_commodity_costs.find(commodity)->second;

			this->get_economy()->change_stored_commodity(commodity, -cost_change);
		}

		if (transporter_type->get_wealth_cost() > 0) {
			const int wealth_cost_change = this->get_transporter_type_wealth_cost(transporter_type, count) - this->get_transporter_type_wealth_cost(transporter_type, old_count);
			this->get_economy()->change_wealth(-wealth_cost_change);
		}
	}
}

bool country_game_data::can_increase_transporter_recruitment(const transporter_type *transporter_type) const
{
	if (this->get_best_transporter_category_type(transporter_type->get_category()) != transporter_type) {
		return false;
	}

	const int old_count = this->get_transporter_recruitment_count(transporter_type);
	const int new_count = old_count + 1;
	const commodity_map<int> old_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, old_count);
	const commodity_map<int> new_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, new_count);

	for (const auto &[commodity, cost] : new_commodity_costs) {
		assert_throw(commodity->is_storable());

		const int cost_change = cost - old_commodity_costs.find(commodity)->second;

		if (this->get_economy()->get_stored_commodity(commodity) < cost_change) {
			return false;
		}
	}

	if (transporter_type->get_wealth_cost() > 0) {
		const int wealth_cost_change = this->get_transporter_type_wealth_cost(transporter_type, new_count) - this->get_transporter_type_wealth_cost(transporter_type, old_count);

		if (this->get_economy()->get_wealth() < wealth_cost_change) {
			return false;
		}
	}

	return true;
}

void country_game_data::increase_transporter_recruitment(const transporter_type *transporter_type)
{
	try {
		assert_throw(this->can_increase_transporter_recruitment(transporter_type));

		this->change_transporter_recruitment_count(transporter_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error increasing recruitment of the \"{}\" transporter type for country \"{}\".", transporter_type->get_identifier(), this->country->get_identifier())));
	}
}

bool country_game_data::can_decrease_transporter_recruitment(const transporter_type *transporter_type) const
{
	if (this->get_transporter_recruitment_count(transporter_type) == 0) {
		return false;
	}

	return true;
}

void country_game_data::decrease_transporter_recruitment(const transporter_type *transporter_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_transporter_recruitment(transporter_type));

		this->change_transporter_recruitment_count(transporter_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error decreasing recruitment of the \"{}\" transporter type for country \"{}\".", transporter_type->get_identifier(), this->country->get_identifier())));
	}
}

int country_game_data::get_transporter_type_cost_modifier(const transporter_type *transporter_type) const
{
	//FIXME: implement cost modifiers for transporters

	Q_UNUSED(transporter_type);

	return 0;
}

int country_game_data::get_transporter_type_wealth_cost(const transporter_type *transporter_type, const int quantity) const
{
	int wealth_cost = transporter_type->get_wealth_cost() * quantity;

	const int cost_modifier = this->get_transporter_type_cost_modifier(transporter_type);
	wealth_cost *= 100 + cost_modifier;
	wealth_cost /= 100;

	if (transporter_type->get_wealth_cost() > 0 && quantity > 0) {
		wealth_cost = std::max(wealth_cost, 1);
	}

	return wealth_cost;
}

commodity_map<int> country_game_data::get_transporter_type_commodity_costs(const transporter_type *transporter_type, const int quantity) const
{
	commodity_map<int> commodity_costs = transporter_type->get_commodity_costs();

	for (auto &[commodity, cost_int] : commodity_costs) {
		assert_throw(commodity->is_storable());

		centesimal_int cost(cost_int);
		cost *= quantity;

		const int cost_modifier = this->get_transporter_type_cost_modifier(transporter_type);
		cost *= 100 + cost_modifier;
		cost /= 100;

		cost_int = cost.to_int();

		if (cost_modifier < 0 && cost.get_fractional_value() > 0) {
			cost_int += 1;
		}

		if (quantity > 0) {
			cost_int = std::max(cost_int, 1);
		}
	}

	return commodity_costs;
}

QVariantList country_game_data::get_transporter_type_commodity_costs_qvariant_list(const transporter_type *transporter_type, const int quantity) const
{
	return archimedes::map::to_qvariant_list(this->get_transporter_type_commodity_costs(transporter_type, quantity));
}

const transporter_type *country_game_data::get_best_transporter_category_type(const transporter_category category, const culture *culture) const
{
	const transporter_type *best_type = nullptr;
	int best_score = -1;

	for (const transporter_class *transporter_class : transporter_class::get_all()) {
		if (transporter_class->get_category() != category) {
			continue;
		}

		const transporter_type *type = culture->get_transporter_class_type(transporter_class);

		if (type == nullptr) {
			continue;
		}

		if (type->get_required_technology() != nullptr && !this->get_technology()->has_technology(type->get_required_technology())) {
			continue;
		}

		bool upgrade_is_available = false;
		for (const transporter_type *upgrade : type->get_upgrades()) {
			if (culture->get_transporter_class_type(upgrade->get_transporter_class()) != upgrade) {
				continue;
			}

			if (upgrade->get_required_technology() != nullptr && !this->get_technology()->has_technology(upgrade->get_required_technology())) {
				continue;
			}

			upgrade_is_available = true;
			break;
		}

		if (upgrade_is_available) {
			continue;
		}

		const int score = type->get_score();

		if (score > best_score) {
			best_type = type;
		}
	}

	return best_type;
}

const transporter_type *country_game_data::get_best_transporter_category_type(const transporter_category category) const
{
	return this->get_best_transporter_category_type(category, this->country->get_culture());
}

void country_game_data::set_transporter_type_stat_modifier(const transporter_type *type, const transporter_stat stat, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_transporter_type_stat_modifier(type, stat);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->transporter_type_stat_modifiers[type].erase(stat);

		if (this->transporter_type_stat_modifiers[type].empty()) {
			this->transporter_type_stat_modifiers.erase(type);
		}
	} else {
		this->transporter_type_stat_modifiers[type][stat] = value;
	}

	const centesimal_int difference = value - old_value;
	for (const qunique_ptr<transporter> &transporter : this->transporters) {
		if (transporter->get_type() != type) {
			continue;
		}

		transporter->change_stat(stat, difference);
	}
}

void country_game_data::set_population_type_modifier_multiplier(const population_type *type, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_population_type_modifier_multiplier(type);

	if (value == old_value) {
		return;
	}

	assert_throw(type->get_country_modifier() != nullptr);

	if (value == 1) {
		this->population_type_modifier_multipliers.erase(type);
	} else {
		this->population_type_modifier_multipliers[type] = value;
	}

	const int population_type_count = this->get_population()->get_type_count(type);
	const centesimal_int &max_modifier_multiplier = type->get_max_modifier_multiplier();

	type->get_country_modifier()->apply(this->country, -centesimal_int::min(population_type_count * old_value, max_modifier_multiplier));
	type->get_country_modifier()->apply(this->country, centesimal_int::min(population_type_count * value, max_modifier_multiplier));
}

void country_game_data::set_building_class_cost_efficiency_modifier(const building_class *building_class, const int value)
{
	if (value == this->get_building_class_cost_efficiency_modifier(building_class)) {
		return;
	}

	if (value == 0) {
		this->building_class_cost_efficiency_modifiers.erase(building_class);
	} else {
		this->building_class_cost_efficiency_modifiers[building_class] = value;
	}
}

bool country_game_data::is_tile_explored(const QPoint &tile_pos) const
{
	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_province() != nullptr && this->explored_provinces.contains(tile->get_province())) {
		return true;
	}

	if (this->explored_tiles.contains(tile_pos)) {
		return true;
	}

	return false;
}

bool country_game_data::is_province_discovered(const province *province) const
{
	//get whether the province has been at least partially explored
	const province_map_data *province_map_data = province->get_map_data();
	if (!province_map_data->is_on_map()) {
		return false;
	}

	if (this->is_province_explored(province)) {
		return true;
	}

	if (!this->explored_tiles.empty()) {
		for (const QPoint &tile_pos : province_map_data->get_tiles()) {
			if (this->explored_tiles.contains(tile_pos)) {
				return true;
			}
		}
	}

	return false;
}

bool country_game_data::is_region_discovered(const region *region) const
{
	for (const province *province : region->get_provinces()) {
		const province_game_data *province_game_data = province->get_game_data();
		if (!province_game_data->is_on_map()) {
			continue;
		}

		if (this->is_province_explored(province)) {
			return true;
		}
	}

	//go through the explored tiles instead of each province's tiles, as this is likely faster
	for (const QPoint &tile_pos : this->explored_tiles) {
		const tile *tile = map::get()->get_tile(tile_pos);
		if (tile->get_province() != nullptr && vector::contains(region->get_provinces(), tile->get_province())) {
			return true;
		}
	}

	return false;
}

void country_game_data::explore_tile(const QPoint &tile_pos)
{
	this->explored_tiles.insert(tile_pos);

	if (this->country == game::get()->get_player_country()) {
		map::get()->update_minimap_rect(QRect(tile_pos, QSize(1, 1)));
		game::get()->set_exploration_changed();
		emit map::get()->tile_exploration_changed(tile_pos);
	}

	const tile *tile = map::get()->get_tile(tile_pos);
	const metternich::country *tile_owner = tile->get_owner();
	const resource *tile_resource = tile->get_resource();

	if (tile_owner != nullptr && tile_owner != this->country && !this->is_country_known(tile_owner)) {
		this->add_known_country(tile_owner);
	}

	if (tile_resource != nullptr && tile->is_resource_discovered() && tile_resource->get_discovery_technology() != nullptr) {
		if (this->get_technology()->can_gain_technology(tile_resource->get_discovery_technology())) {
			this->get_technology()->add_technology(tile_resource->get_discovery_technology());

			if (game::get()->is_running()) {
				emit get_technology()->technology_researched(tile_resource->get_discovery_technology());
			}
		}
	}

	if (tile->get_province() != nullptr) {
		//add the tile's province to the explored provinces if all of its tiles have been explored
		const province_map_data *province_map_data = tile->get_province()->get_map_data();
		if (this->explored_tiles.size() >= province_map_data->get_tiles().size()) {
			for (const QPoint &province_tile_pos : province_map_data->get_tiles()) {
				if (!this->explored_tiles.contains(province_tile_pos)) {
					return;
				}
			}

			//add the province to the explored provinces and remove its tiles from the explored tiles, as they no longer need to be taken into account separately
			this->explore_province(tile->get_province());
		}
	}
}

void country_game_data::explore_province(const province *province)
{
	if (this->explored_provinces.contains(province)) {
		return;
	}

	const province_game_data *province_game_data = province->get_game_data();
	assert_throw(province_game_data->is_on_map());

	const metternich::country *province_owner = province_game_data->get_owner();

	if (province_owner != nullptr && province_owner != this->country && !this->is_country_known(province_owner)) {
		this->add_known_country(province_owner);
	}

	const province_map_data *province_map_data = province->get_map_data();

	if (!this->explored_tiles.empty()) {
		for (const QPoint &tile_pos : province_map_data->get_tiles()) {
			if (this->explored_tiles.contains(tile_pos)) {
				this->explored_tiles.erase(tile_pos);
			}
		}
	}

	this->explored_provinces.insert(province);

	if (this->country == game::get()->get_player_country()) {
		map::get()->update_minimap_rect(province_map_data->get_territory_rect());
		game::get()->set_exploration_changed();

		for (const QPoint &tile_pos : province_map_data->get_tiles()) {
			emit map::get()->tile_exploration_changed(tile_pos);
		}
	}

	for (const QPoint &tile_pos : province_map_data->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const resource *tile_resource = tile->get_resource();

		if (tile_resource != nullptr && tile->is_resource_discovered() && tile_resource->get_discovery_technology() != nullptr) {
			if (this->get_technology()->can_gain_technology(tile_resource->get_discovery_technology())) {
				this->get_technology()->add_technology(tile_resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit get_technology()->technology_researched(tile_resource->get_discovery_technology());
				}
			}
		}
	}
}

void country_game_data::prospect_tile(const QPoint &tile_pos)
{
	this->prospected_tiles.insert(tile_pos);

	const tile *tile = map::get()->get_tile(tile_pos);
	const resource *tile_resource = tile->get_resource();

	if (tile_resource != nullptr) {
		if (!tile->is_resource_discovered() && (tile_resource->get_required_technology() == nullptr || this->get_technology()->has_technology(tile_resource->get_required_technology()))) {
			map::get()->set_tile_resource_discovered(tile_pos, true);
		}
	}

	emit prospected_tiles_changed();

	if (this->country == game::get()->get_player_country()) {
		emit map::get()->tile_prospection_changed(tile_pos);
	}
}

void country_game_data::reset_tile_prospection(const QPoint &tile_pos)
{
	this->prospected_tiles.erase(tile_pos);

	emit prospected_tiles_changed();

	if (this->country == game::get()->get_player_country()) {
		emit map::get()->tile_prospection_changed(tile_pos);
	}
}

QVariantList country_game_data::get_active_journal_entries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_active_journal_entries());
}

void country_game_data::add_active_journal_entry(const journal_entry *journal_entry)
{
	this->active_journal_entries.push_back(journal_entry);

	if (journal_entry->get_active_modifier() != nullptr) {
		journal_entry->get_active_modifier()->apply(this->country);
	}

	for (const building_type *building : journal_entry->get_built_buildings_with_requirements()) {
		this->get_ai()->change_building_desire_modifier(building, journal_entry::ai_building_desire_modifier);
	}

	for (const auto &[settlement, buildings] : journal_entry->get_built_settlement_buildings_with_requirements()) {
		for (const building_type *building : buildings) {
			this->get_ai()->change_settlement_building_desire_modifier(settlement, building, journal_entry::ai_building_desire_modifier);
		}
	}
}

void country_game_data::remove_active_journal_entry(const journal_entry *journal_entry)
{
	std::erase(this->active_journal_entries, journal_entry);

	if (journal_entry->get_active_modifier() != nullptr) {
		journal_entry->get_active_modifier()->remove(this->country);
	}

	for (const building_type *building : journal_entry->get_built_buildings_with_requirements()) {
		this->get_ai()->change_building_desire_modifier(building, -journal_entry::ai_building_desire_modifier);
	}

	for (const auto &[settlement, buildings] : journal_entry->get_built_settlement_buildings_with_requirements()) {
		for (const building_type *building : buildings) {
			this->get_ai()->change_settlement_building_desire_modifier(settlement, building, -journal_entry::ai_building_desire_modifier);
		}
	}
}

QVariantList country_game_data::get_inactive_journal_entries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_inactive_journal_entries());
}

QVariantList country_game_data::get_finished_journal_entries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_finished_journal_entries());
}

void country_game_data::check_journal_entries(const bool ignore_effects, const bool ignore_random_chance)
{
	const read_only_context ctx(this->country);

	bool changed = false;

	//check if any journal entry has become potentially available
	if (this->check_potential_journal_entries()) {
		changed = true;
	}

	if (this->check_inactive_journal_entries()) {
		changed = true;
	}

	if (this->check_active_journal_entries(ctx, ignore_effects, ignore_random_chance)) {
		changed = true;
	}

	if (changed) {
		emit journal_entries_changed();
	}
}

bool country_game_data::check_potential_journal_entries()
{
	bool changed = false;

	for (const journal_entry *journal_entry : journal_entry::get_all()) {
		if (vector::contains(this->get_active_journal_entries(), journal_entry)) {
			continue;
		}

		if (vector::contains(this->get_inactive_journal_entries(), journal_entry)) {
			continue;
		}

		if (vector::contains(this->get_finished_journal_entries(), journal_entry)) {
			continue;
		}

		if (!journal_entry->check_preconditions(this->country)) {
			continue;
		}

		this->inactive_journal_entries.push_back(journal_entry);
		changed = true;
	}

	return changed;
}

bool country_game_data::check_inactive_journal_entries()
{
	bool changed = false;

	const std::vector<const journal_entry *> inactive_entries = this->get_inactive_journal_entries();

	for (const journal_entry *journal_entry : inactive_entries) {
		if (!journal_entry->check_preconditions(this->country)) {
			std::erase(this->inactive_journal_entries, journal_entry);
			changed = true;
			continue;
		}

		if (!journal_entry->check_conditions(this->country)) {
			continue;
		}

		std::erase(this->inactive_journal_entries, journal_entry);

		this->add_active_journal_entry(journal_entry);

		changed = true;
	}

	return changed;
}

bool country_game_data::check_active_journal_entries(const read_only_context &ctx, const bool ignore_effects, const bool ignore_random_chance)
{
	bool changed = false;

	const std::vector<const journal_entry *> active_entries = this->get_active_journal_entries();

	for (const journal_entry *journal_entry : active_entries) {
		if (!journal_entry->check_preconditions(this->country)) {
			this->remove_active_journal_entry(journal_entry);
			changed = true;
			continue;
		}

		if (!journal_entry->check_conditions(this->country)) {
			this->remove_active_journal_entry(journal_entry);
			this->inactive_journal_entries.push_back(journal_entry);
			changed = true;
			continue;
		}

		if (journal_entry->check_completion_conditions(this->country, ignore_random_chance)) {
			this->remove_active_journal_entry(journal_entry);
			this->finished_journal_entries.push_back(journal_entry);
			if (!ignore_effects) {
				if (journal_entry->get_completion_effects() != nullptr) {
					context effects_ctx(this->country);
					journal_entry->get_completion_effects()->do_effects(this->country, effects_ctx);
				}

				if (game::get()->is_running()) {
					emit journal_entry_completed(journal_entry);
				}
			}

			if (journal_entry->get_completion_modifier() != nullptr) {
				journal_entry->get_completion_modifier()->apply(this->country);
			}

			changed = true;
		} else if (journal_entry->get_failure_conditions() != nullptr && journal_entry->get_failure_conditions()->check(this->country, ctx)) {
			this->remove_active_journal_entry(journal_entry);
			this->finished_journal_entries.push_back(journal_entry);
			if (journal_entry->get_failure_effects() != nullptr && !ignore_effects) {
				context effects_ctx(this->country);
				journal_entry->get_failure_effects()->do_effects(this->country, effects_ctx);

				if (this->country == game::get()->get_player_country()) {
					engine_interface::get()->add_notification(journal_entry->get_name(), journal_entry->get_portrait(), std::format("{}{}{}", journal_entry->get_description(), (!journal_entry->get_description().empty() ? "\n\n" : ""), journal_entry->get_failure_effects()->get_effects_string(this->country, ctx)));
				}
			}
			changed = true;
		}
	}

	return changed;
}

void country_game_data::set_free_building_class_count(const building_class *building_class, const int value)
{
	const int old_value = this->get_free_building_class_count(building_class);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_building_class_counts.erase(building_class);
	} else if (old_value == 0) {
		this->free_building_class_counts[building_class] = value;

		for (const province *province : this->get_provinces()) {
			for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
				if (!settlement->get_game_data()->is_built()) {
					continue;
				}

				settlement->get_game_data()->check_free_buildings();
			}
		}
	}
}

void country_game_data::set_free_consulate_count(const consulate *consulate, const int value)
{
	const int old_value = this->get_free_consulate_count(consulate);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_consulate_counts.erase(consulate);
	} else if (old_value == 0) {
		this->free_consulate_counts[consulate] = value;

		for (const metternich::country *known_country : this->get_known_countries()) {
			const metternich::consulate *current_consulate = this->get_consulate(known_country);
			if (current_consulate == nullptr || current_consulate->get_level() < consulate->get_level()) {
				this->set_consulate(known_country, consulate);
			}
		}
	}
}

int country_game_data::get_min_income() const
{
	int min_income = 0;

	for (const province *province : this->get_provinces()) {
		min_income += province->get_game_data()->get_min_income();
	}

	return min_income;
}

int country_game_data::get_max_income() const
{
	int max_income = 0;

	for (const province *province : this->get_provinces()) {
		max_income += province->get_game_data()->get_max_income();
	}

	return max_income;
}

int country_game_data::get_domain_maintenance_cost() const
{
	const int province_count = this->get_province_count();
	assert_throw(province_count > 0);
	return defines::get()->get_domain_maintenance_cost_for_province_count(province_count);
}

int country_game_data::get_maintenance_cost() const
{
	int maintenance_cost = this->get_domain_maintenance_cost();

	for (const qunique_ptr<military_unit> &military_unit : this->get_military()->get_military_units()) {
		const auto find_iterator = military_unit->get_type()->get_maintenance_commodity_costs().find(defines::get()->get_wealth_commodity());
		if (find_iterator != military_unit->get_type()->get_maintenance_commodity_costs().end()) {
			maintenance_cost += find_iterator->second;
		}
	}

	return maintenance_cost;
}

}
