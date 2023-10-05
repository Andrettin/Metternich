#include "metternich.h"

#include "country/country_game_data.h"

#include "character/advisor_category.h"
#include "character/advisor_type.h"
#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "country/consulate.h"
#include "country/country.h"
#include "country/country_turn_data.h"
#include "country/country_type.h"
#include "country/culture.h"
#include "country/diplomacy_state.h"
#include "country/government_type.h"
#include "country/journal_entry.h"
#include "country/policy.h"
#include "country/religion.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"
#include "economy/income_transaction_type.h"
#include "economy/production_type.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/country_building_slot.h"
#include "infrastructure/improvement.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/settlement_type.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/phenotype.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "script/opinion_modifier.h"
#include "technology/technology.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "ui/portrait.h"
#include "unit/civilian_unit.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
#include "util/size_util.h"
#include "util/string_util.h"
#include "util/thread_pool.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

#include "xbrz.h"

namespace metternich {

country_game_data::country_game_data(metternich::country *country) : country(country)
{
	connect(this, &country_game_data::rank_changed, this, &country_game_data::type_name_changed);

	for (const commodity *commodity : commodity::get_all()) {
		if (commodity->get_required_technology() != nullptr) {
			continue;
		}

		this->add_available_commodity(commodity);

		if (commodity->is_tradeable()) {
			this->add_tradeable_commodity(commodity);
		}
	}

	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::type_count_changed, this, &country_game_data::on_population_type_count_changed);
}

country_game_data::~country_game_data()
{
}

void country_game_data::do_turn()
{
	try {
		for (const province *province : this->get_provinces()) {
			province->get_game_data()->do_turn();
		}

		this->do_production();
		this->do_research();
		this->do_population_growth();
		this->do_consumption();
		this->do_construction();
		this->do_inflation();

		for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
			civilian_unit->do_turn();
		}

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			military_unit->do_turn();
		}

		this->decrement_scripted_modifiers();

		this->check_journal_entries();
		this->do_events();

		this->check_characters();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to process turn for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_production()
{
	try {
		//FIXME: add preference for production being automatically assigned for person players
		if (this->is_ai()) {
			this->assign_production();
		}

		for (const auto &[commodity, output] : this->get_commodity_outputs()) {
			if (!commodity->is_storable()) {
				assert_throw(output >= 0);
				continue;
			}

			int final_output = output;

			if (commodity == defines::get()->get_research_commodity() && this->get_current_research() != nullptr) {
				final_output *= 100 + this->get_category_research_modifier(this->get_current_research()->get_category());
				final_output /= 100;
			}

			this->change_stored_commodity(commodity, final_output);
		}

		//decrease consumption of commodities for which we no longer have enough in storage
		while (this->get_wealth_income() < 0 && (this->get_wealth_income() * -1) > this->get_wealth_with_credit()) {
			this->decrease_wealth_consumption(false);
		}

		const std::vector<const commodity *> input_commodities = archimedes::map::get_keys(this->get_commodity_inputs());

		for (const commodity *commodity : input_commodities) {
			if (!commodity->is_storable() || commodity->is_negative_allowed()) {
				continue;
			}

			while (this->get_commodity_input(commodity) > this->get_stored_commodity(commodity)) {
				this->decrease_commodity_consumption(commodity, false);
			}
		}

		//reduce inputs from the storage for the next turn (for production this turn it had already been subtracted)
		if (this->get_wealth_income() != 0) {
			this->change_wealth(this->get_wealth_income());
		}

		for (const auto &[commodity, input] : this->get_commodity_inputs()) {
			try {
				if (!commodity->is_storable()) {
					const int output = this->get_commodity_output(commodity);
					if (input > output) {
						throw std::runtime_error(std::format("Input for non-storable commodity \"{}\" ({}) is greater than its output ({}).", commodity->get_identifier(), input, output));
					}
					continue;
				}

				this->change_stored_commodity(commodity, -input);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Error processing input storage reduction for commodity \"" + commodity->get_identifier()  + "\"."));
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing production for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_research()
{
	try {
		if (this->get_gain_technologies_known_by_others_count() > 0) {
			this->gain_technologies_known_by_others();
		}

		assert_throw(this->free_technology_count >= 0);

		const technology *researched_technology = this->get_current_research();

		if (researched_technology == nullptr) {
			if (this->free_technology_count > 0) {
				this->gain_free_technology();
			} else {
				if (this->get_commodity_output(defines::get()->get_research_commodity()) > 0 || this->get_stored_commodity(defines::get()->get_research_commodity()) > 0) {
					this->choose_current_research();
				}
			}
			return;
		}

		const int technology_cost = researched_technology->get_cost() * this->get_research_cost_modifier() / 100;
		if (this->get_stored_commodity(defines::get()->get_research_commodity()) >= technology_cost || this->free_technology_count > 0) {
			if (this->free_technology_count > 0) {
				--this->free_technology_count;
			} else {
				this->change_stored_commodity(defines::get()->get_research_commodity(), -technology_cost);
			}

			this->on_technology_researched(researched_technology);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing research for country \"" + this->country->get_identifier() + "\"."));
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
		const int available_housing = std::max(0, this->get_available_housing());

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
	const commodity_map<int> stored_commodities = this->get_stored_commodities();

	for (const auto &[commodity, quantity] : stored_commodities) {
		if (commodity->is_food()) {
			const int consumed_food = std::min(remaining_food_consumption, quantity);
			this->change_stored_commodity(commodity, -consumed_food);

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
		this->decrease_population();
		++starvation_count;

		if (this->get_food_consumption() == 0) {
			this->set_population_growth(0);
			break;
		}
	}

	if (starvation_count > 0 && this->country == game::get()->get_player_country()) {
		const bool plural = starvation_count > 1;

		const portrait *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

		engine_interface::get()->add_notification("Starvation", interior_minister_portrait, std::format("Your Excellency, I regret to inform you that {} {} of our population {} starved to death.", number::to_formatted_string(starvation_count), (plural ? "units" : "unit"), (plural ? "have" : "has")));
	}
}

void country_game_data::do_consumption()
{
	if (this->get_population_units().empty()) {
		return;
	}

	const std::vector<population_unit *> population_units = vector::shuffled(this->get_population_units());

	for (population_unit *population_unit : population_units) {
		population_unit->set_consumption_fulfilled(true);
	}

	for (const auto &[commodity, consumption] : this->get_commodity_consumptions()) {
		//local consumption is handled separately
		assert_throw(!commodity->is_local());

		int effective_consumption = 0;

		if (commodity->is_storable()) {
			effective_consumption = std::min(consumption.to_int(), this->get_stored_commodity(commodity));
			this->change_stored_commodity(commodity, -effective_consumption);
		} else {
			effective_consumption = std::min(consumption.to_int(), this->get_net_commodity_output(commodity));
		}

		centesimal_int remaining_consumption(consumption.to_int() - effective_consumption);
		if (remaining_consumption == 0) {
			continue;
		}

		//go through population units belonging to the country in random order, set whether their consumption was fulfilled
		for (population_unit *population_unit : population_units) {
			const centesimal_int pop_consumption = population_unit->get_type()->get_commodity_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_consumption_fulfilled(false);
			remaining_consumption -= pop_consumption;

			if (remaining_consumption <= 0) {
				break;
			}
		}
	}

	for (const province *province : this->get_provinces()) {
		for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
			if (!settlement->get_game_data()->is_built()) {
				continue;
			}

			settlement->get_game_data()->do_consumption();
		}
	}

	static const centesimal_int militancy_change_for_unfulfilled_consumption(0.1);
	static const centesimal_int militancy_change_for_fulfilled_consumption(-0.1);

	for (population_unit *population_unit : population_units) {
		if (population_unit->is_consumption_fulfilled()) {
			population_unit->change_militancy(militancy_change_for_fulfilled_consumption);
		} else {
			population_unit->change_militancy(militancy_change_for_unfulfilled_consumption);
		}
	}

	//FIXME: make population units which couldn't have their consumption fulfilled be unhappy/refuse to work for the turn (and possibly demote when demotion is implemented)

	//for minor nations, consume commodities demanded by the population
	//this is to prevent commodities bought by minor nations from infinitely stockpiling up
	if (!this->country->is_great_power()) {
		for (const auto &[commodity, demand] : this->get_commodity_demands()) {
			const int demand_int = demand.to_int();
			const int consumed_quantity = std::min(this->get_stored_commodity(commodity), demand_int);
			if (consumed_quantity > 0) {
				this->change_stored_commodity(commodity, -consumed_quantity);
				this->change_wealth(consumed_quantity * game::get()->get_price(commodity));
			}
		}
	}
}

void country_game_data::do_cultural_change()
{
	static constexpr int cultural_derivation_chance = 1;

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

		if (!potential_cultures.empty() && random::get()->generate(100) < cultural_derivation_chance) {
			const metternich::culture *new_culture = vector::get_random(potential_cultures);
			population_unit->set_culture(new_culture);
		}
	}
}

void country_game_data::do_construction()
{
	try {
		for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
			if (building_slot->get_under_construction_building() != nullptr) {
				building_slot->set_building(building_slot->get_under_construction_building());
				building_slot->set_under_construction_building(nullptr);
			}

			if (building_slot->is_expanding()) {
				building_slot->expand();
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing construction for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_trade()
{
	try {
		if (this->is_under_anarchy()) {
			return;
		}

		//get the known countries and sort them by priority
		std::vector<const metternich::country *> countries = container::to_vector(this->get_known_countries());
		std::sort(countries.begin(), countries.end(), [&](const metternich::country *lhs, const metternich::country *rhs) {
			if (this->is_vassal_of(lhs) != this->is_vassal_of(rhs)) {
				return this->is_vassal_of(lhs);
			}

			if (this->is_any_vassal_of(lhs) != this->is_any_vassal_of(rhs)) {
				return this->is_any_vassal_of(lhs);
			}

			//give trade priority by opinion-weighted prestige
			const int lhs_opinion_weighted_prestige = this->get_opinion_weighted_prestige_for(lhs);
			const int rhs_opinion_weighted_prestige = this->get_opinion_weighted_prestige_for(rhs);

			if (lhs_opinion_weighted_prestige != rhs_opinion_weighted_prestige) {
				return lhs_opinion_weighted_prestige > rhs_opinion_weighted_prestige;
			}

			return lhs->get_identifier() < rhs->get_identifier();
		});

		commodity_map<int> offers = this->get_offers();
		for (auto &[commodity, offer] : offers) {
			const int price = game::get()->get_price(commodity);

			for (const metternich::country *other_country : countries) {
				country_game_data *other_country_game_data = other_country->get_game_data();

				const int bid = other_country_game_data->get_bid(commodity);
				if (bid == 0) {
					continue;
				}

				int sold_quantity = std::min(offer, bid);
				sold_quantity = std::min(sold_quantity, other_country_game_data->get_wealth_with_credit() / price);

				if (sold_quantity <= 0) {
					continue;
				}

				this->change_stored_commodity(commodity, -sold_quantity);
				const int sale_income = price * sold_quantity;
				this->change_wealth(sale_income);
				this->country->get_turn_data()->add_income_transaction(income_transaction_type::sale, sale_income, commodity, sold_quantity, other_country);

				other_country_game_data->change_stored_commodity(commodity, sold_quantity);
				const int purchase_expense = other_country_game_data->get_inflated_value(price * sold_quantity);
				other_country_game_data->change_wealth(-purchase_expense);
				other_country->get_turn_data()->add_expense_transaction(expense_transaction_type::purchase, purchase_expense, commodity, sold_quantity, this->country);

				offer -= sold_quantity;

				this->change_offer(commodity, -sold_quantity);
				other_country_game_data->change_bid(commodity, -sold_quantity);

				//improve relations between the two countries after they traded
				this->change_base_opinion(other_country, 1);
				other_country_game_data->change_base_opinion(this->country, 1);

				if (offer == 0) {
					break;
				}
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing trade for country \"{}\".", this->country->get_identifier())));
	}
}


void country_game_data::do_inflation()
{
	try {
		if (this->is_under_anarchy()) {
			return;
		}

		this->country->get_turn_data()->calculate_inflation();
		this->change_inflation(this->country->get_turn_data()->get_total_inflation_change());
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing trade for country \"{}\".", this->country->get_identifier())));
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

	const bool is_last_turn_of_year = (game::get()->get_date().date().month() + defines::get()->get_months_per_turn()) > 12;

	if (is_last_turn_of_year) {
		country_event::check_events_for_scope(this->country, event_trigger::yearly_pulse);
	}

	country_event::check_events_for_scope(this->country, event_trigger::quarterly_pulse);
}

void country_game_data::do_ai_turn()
{
	//build buildings
	building_type_map<int> ai_building_desires;
	std::vector<const building_type *> ai_desired_buildings;
	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		if (!building_slot->is_available()) {
			continue;
		}

		const building_type *buildable_building = building_slot->get_buildable_building();

		if (buildable_building == nullptr) {
			continue;
		}

		if (building_slot->is_expanding() || building_slot->get_under_construction_building() != nullptr) {
			continue;
		}

		int ai_building_desire = 0;
		ai_building_desire += this->get_ai_building_desire_modifier(buildable_building);

		if (ai_building_desire <= 0) {
			continue;
		}

		ai_building_desires[buildable_building] = ai_building_desire;
		ai_desired_buildings.push_back(buildable_building);
	}

	std::sort(ai_desired_buildings.begin(), ai_desired_buildings.end(), [&](const building_type *lhs, const building_type *rhs) {
		const int lhs_priority = ai_building_desires[lhs];
		const int rhs_priority = ai_building_desires[rhs];
		if (lhs_priority != rhs_priority) {
			return lhs_priority > rhs_priority;
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const building_type *ai_desired_building : ai_desired_buildings) {
		country_building_slot *building_slot = this->get_building_slot(ai_desired_building->get_slot_type());
		assert_throw(building_slot != nullptr);

		building_slot->build_building(ai_desired_building);
	}

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_ai_turn();
	}

	for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
		civilian_unit->do_ai_turn();
	}

	for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
		military_unit->do_ai_turn();
	}

	for (size_t i = 0; i < this->civilian_units.size();) {
		civilian_unit *civilian_unit = this->civilian_units.at(i).get();
		if (civilian_unit->is_busy()) {
			++i;
		} else {
			//if the civilian unit is idle, this means that nothing was found for it to do above; in that case, disband it
			civilian_unit->disband();
		}
	}

	this->assign_trade_orders();
}

bool country_game_data::is_ai() const
{
	return this->country != game::get()->get_player_country();
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

	if (game::get()->is_running()) {
		emit religion_changed();
	}
}

void country_game_data::set_overlord(const metternich::country *overlord)
{
	if (overlord == this->get_overlord()) {
		return;
	}

	if (this->overlord != nullptr) {
		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, -count);
		}
	}

	this->overlord = overlord;

	if (this->overlord != nullptr) {
		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count);
		}
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

bool country_game_data::is_true_great_power() const
{
	if (this->country->get_type() != country_type::great_power) {
		return false;
	}

	return this->get_rank() < country::max_great_powers;
}

bool country_game_data::is_secondary_power() const
{
	//a country is a secondary power if its type is great power, and it is not high-ranking enough to be considered a true great power
	if (this->country->get_type() != country_type::great_power) {
		return false;
	}

	return this->get_rank() >= country::max_great_powers;
}

bool country_game_data::is_colony() const
{
	if (this->get_overlord() == nullptr) {
		return false;
	}

	return this->get_diplomacy_state(this->get_overlord()) == diplomacy_state::colony;
}

std::string country_game_data::get_type_name() const
{
	switch (this->country->get_type()) {
		case country_type::great_power:
			if (this->get_overlord() != nullptr) {
				return "Subject Power";
			}

			if (this->is_secondary_power()) {
				return "Secondary Power";
			}

			return "Great Power";
		case country_type::minor_nation:
			return "Minor Nation";
		case country_type::tribe:
			return "Tribe";
		default:
			assert_throw(false);
	}

	return std::string();
}

std::string country_game_data::get_vassalage_type_name() const
{
	if (this->get_overlord() == nullptr) {
		return std::string();
	}

	switch (this->get_diplomacy_state(this->get_overlord())) {
		case diplomacy_state::vassal:
			return "Vassal";
		case diplomacy_state::personal_union_subject:
			return "Personal Union Subject";
		case diplomacy_state::colony:
			return "Colony";
		default:
			assert_throw(false);
	}

	return std::string();
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
			if (!tile->is_resource_discovered()) {
				assert_throw(resource->get_required_technology() != nullptr);

				if (this->has_technology(resource->get_required_technology())) {
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

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->change_resource_count(resource, count * multiplier);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count * multiplier);
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);

		for (const auto &[commodity, output] : tile->get_commodity_outputs()) {
			if (commodity->is_local()) {
				continue;
			}

			this->change_commodity_output(commodity, output * multiplier);
		}
	}

	for (const auto &[terrain, count] : province_game_data->get_tile_terrain_counts()) {
		this->change_tile_terrain_count(terrain, count * multiplier);
	}

	for (const site *settlement : province_game_data->get_settlement_sites()) {
		if (!settlement->get_game_data()->is_built()) {
			continue;
		}

		for (const qunique_ptr<settlement_building_slot> &building_slot : settlement->get_game_data()->get_building_slots()) {
			const building_type *building = building_slot->get_building();
			if (building != nullptr) {
				assert_throw(building->is_provincial());
				this->change_settlement_building_count(building, 1 * multiplier);
			}
		}

		this->change_housing(settlement->get_game_data()->get_housing() * multiplier);
	}

	for (const auto &[resource, commodity_map] : this->improved_resource_commodity_bonuses) {
		for (const auto &[commodity, value] : commodity_map) {
			province_game_data->change_improved_resource_commodity_bonus(resource, commodity, value * multiplier);
		}
	}

	for (const auto &[commodity, threshold_map] : this->commodity_bonuses_for_tile_thresholds) {
		for (const auto &[threshold, value] : threshold_map) {
			province_game_data->change_commodity_bonus_for_tile_threshold(commodity, threshold, value * multiplier);
		}
	}
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
		capital->get_game_data()->get_province()->get_game_data()->set_provincial_capital(capital);
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

				if (best_capital->get_game_data()->get_settlement_type()->get_free_resource_building_level() >= settlement_game_data->get_settlement_type()->get_free_resource_building_level()) {
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
		if (building->get_country_modifier() != nullptr) {
			building->get_country_modifier()->apply(this->country, centesimal_int(-count) / old_settlement_count);
		}
	}

	if (this->get_settlement_count() != 0) {
		for (const auto &[building, count] : this->settlement_building_counts) {
			if (building->get_country_modifier() != nullptr) {
				//reapply the settlement building's country modifier with the updated province count
				building->get_country_modifier()->apply(this->country, centesimal_int(count) / this->get_settlement_count());
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
		QtConcurrent::run([this]() -> QCoro::Task<void> {
			co_await this->create_diplomatic_map_image();
		}).waitForFinished();
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

QVariantList country_game_data::get_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_resource_counts());
}

QVariantList country_game_data::get_vassal_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_vassal_resource_counts());
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

	if (this->get_gain_technologies_known_by_others_count() > 0) {
		this->gain_technologies_known_by_others();
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

	//minor nations cannot have diplomacy with one another
	assert_throw(this->country->is_great_power() || other_country->is_great_power());

	switch (state) {
		case diplomacy_state::alliance:
			//only great powers can ally between themselves
			assert_throw(this->country->is_great_power() && other_country->is_great_power());
			break;
		case diplomacy_state::non_aggression_pact:
			//non-aggression pacts can only be formed between a great power and a minor nation
			assert_throw(this->country->is_great_power() != other_country->is_great_power());
			break;
		default:
			break;
	}

	if (is_vassalage_diplomacy_state(state)) {
		//only great powers can have vassals
		assert_throw(other_country->is_great_power());

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
			emit vassalage_type_name_changed();
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
		QtConcurrent::run([this, state]() -> QCoro::Task<void> {
			co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, state);
		}).waitForFinished();
	}
}

QString country_game_data::get_diplomacy_state_diplomatic_map_suffix(metternich::country *other_country) const
{
	if (other_country == this->country || this->is_any_overlord_of(other_country) || this->is_any_vassal_of(other_country)) {
		return "empire";
	}

	return QString::fromStdString(enum_converter<diplomacy_state>::to_string(this->get_diplomacy_state(other_country)));
}

bool country_game_data::at_war() const
{
	return this->diplomacy_state_counts.contains(diplomacy_state::war);
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
	const int prestige = std::max(1, other->get_game_data()->get_stored_commodity(defines::get()->get_prestige_commodity()));

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

QVariantList country_game_data::get_colonies_qvariant_list() const
{
	std::vector<const metternich::country *> colonies;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (diplomacy_state == diplomacy_state::colonial_overlord) {
			colonies.push_back(country);
		}
	}

	return container::to_qvariant_list(colonies);
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

QCoro::Task<void> country_game_data::create_diplomatic_map_image()
{
	const map *map = map::get();

	const int tile_pixel_size = map->get_diplomatic_map_tile_pixel_size();

	assert_throw(this->territory_rect.width() > 0);
	assert_throw(this->territory_rect.height() > 0);

	QImage diplomatic_map_image = QImage(this->territory_rect.size(), QImage::Format_RGBA8888);
	diplomatic_map_image.fill(Qt::transparent);

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

	QImage scaled_diplomatic_map_image;
	QImage scaled_selected_diplomatic_map_image;

	co_await QtConcurrent::run([tile_pixel_size, &diplomatic_map_image, &scaled_diplomatic_map_image, &selected_diplomatic_map_image, &scaled_selected_diplomatic_map_image]() {
		scaled_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(diplomatic_map_image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});

		scaled_selected_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(selected_diplomatic_map_image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	diplomatic_map_image = std::move(scaled_diplomatic_map_image);
	selected_diplomatic_map_image = std::move(scaled_selected_diplomatic_map_image);

	std::vector<QPoint> border_pixels;

	for (int x = 0; x < diplomatic_map_image.width(); ++x) {
		for (int y = 0; y < diplomatic_map_image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = diplomatic_map_image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (diplomatic_map_image.width() - 1) || pixel_pos.y() == (diplomatic_map_image.height() - 1)) {
				border_pixels.push_back(pixel_pos);
				continue;
			}

			if (pixel_color != color) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			bool is_border_pixel = false;
			point::for_each_cardinally_adjacent_until(pixel_pos, [&diplomatic_map_image, &color, &is_border_pixel](const QPoint &adjacent_pos) {
				if (diplomatic_map_image.pixelColor(adjacent_pos).alpha() != 0) {
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
		diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
		selected_diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	co_await QtConcurrent::run([&scale_factor, &diplomatic_map_image, &scaled_diplomatic_map_image, &selected_diplomatic_map_image, &scaled_selected_diplomatic_map_image]() {
		scaled_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});

		scaled_selected_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(selected_diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->diplomatic_map_image = std::move(scaled_diplomatic_map_image);
	this->selected_diplomatic_map_image = std::move(scaled_selected_diplomatic_map_image);

	this->diplomatic_map_image_rect = QRect(this->territory_rect.topLeft() * tile_pixel_size * scale_factor, this->diplomatic_map_image.size());

	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, {});
	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, diplomacy_state::peace);

	for (const auto &[diplomacy_state, count] : this->get_diplomacy_state_counts()) {
		if (!is_vassalage_diplomacy_state(diplomacy_state) && !is_overlordship_diplomacy_state(diplomacy_state)) {
			co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, diplomacy_state);
		}
	}

	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::terrain, {});
	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::cultural, {});
	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::religious, {});

	emit diplomatic_map_image_changed();
}

QCoro::Task<void> country_game_data::create_diplomatic_map_mode_image(const diplomatic_map_mode mode, const std::optional<diplomacy_state> &diplomacy_state)
{
	static const QColor empty_color(Qt::black);
	static constexpr QColor diplomatic_self_color(170, 148, 214);

	const map *map = map::get();

	const int tile_pixel_size = map->get_diplomatic_map_tile_pixel_size();

	assert_throw(this->territory_rect.width() > 0);
	assert_throw(this->territory_rect.height() > 0);

	QImage image(this->territory_rect.size(), QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

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
					if (diplomacy_state.has_value()) {
						color = &defines::get()->get_diplomacy_state_color(diplomacy_state.value());
					} else {
						color = &diplomatic_self_color;
					}
					break;
				case diplomatic_map_mode::terrain:
					color = &tile->get_terrain()->get_color();
					break;
				case diplomatic_map_mode::cultural: {
					const culture *culture = nullptr;

					if (tile->get_settlement() != nullptr && tile->get_settlement()->get_game_data()->get_culture() != nullptr) {
						culture = tile->get_settlement()->get_game_data()->get_culture();
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

	QImage scaled_image;

	co_await QtConcurrent::run([this, tile_pixel_size, &image, &scaled_image]() {
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
			point::for_each_cardinally_adjacent_until(pixel_pos, [this, &image, &is_border_pixel](const QPoint &adjacent_pos) {
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


	QImage &stored_image = (mode == diplomatic_map_mode::diplomatic && diplomacy_state.has_value()) ? this->diplomacy_state_diplomatic_map_images[diplomacy_state.value()] : this->diplomatic_map_mode_images[mode];
	stored_image = std::move(scaled_image);
}

void country_game_data::change_score(const int change)
{
	if (change == 0) {
		return;
	}

	this->score += change;

	emit score_changed();
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
	for (const auto &[commodity, value] : type->get_consumed_commodities()) {
		if (commodity->is_local()) {
			//handled at the settlement level
			continue;
		}

		this->change_commodity_consumption(commodity, value * change);
	}

	//minor nations generate demand in the world market depending on population commodity demand
	if (!this->country->is_great_power()) {
		for (const auto &[commodity, value] : type->get_commodity_demands()) {
			this->change_commodity_demand(commodity, value * change);
		}
	}

	this->change_food_consumption(change);
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
		if (population_unit->get_settlement()->get_game_data()->get_available_housing() <= 0) {
			return true;
		}

		return false;
	});

	const population_unit *population_unit = vector::get_random(potential_base_population_units);
	const metternich::culture *culture = population_unit->get_culture();
	const metternich::religion *religion = population_unit->get_religion();
	const phenotype *phenotype = population_unit->get_phenotype();
	const population_type *population_type = culture->get_population_class_type(this->country->get_default_population_class());
	const site *settlement = population_unit->get_settlement();

	settlement->get_game_data()->create_population_unit(population_type, culture, religion, phenotype);

	this->change_population_growth(-defines::get()->get_population_growth_threshold());
}

void country_game_data::decrease_population()
{
	//disband population unit, if possible
	if (!this->population_units.empty()) {
		population_unit *population_unit = this->choose_starvation_population_unit();
		if (population_unit != nullptr) {
			this->change_population_growth(1);
			population_unit->get_province()->get_game_data()->remove_population_unit(population_unit);
			population_unit->get_settlement()->get_game_data()->pop_population_unit(population_unit);
			return;
		}
	}

	//disband civilian unit, if possible
	civilian_unit *best_civilian_unit = nullptr;

	for (auto it = this->civilian_units.rbegin(); it != this->civilian_units.rend(); ++it) {
		civilian_unit *civilian_unit = it->get();

		if (
			best_civilian_unit == nullptr
			|| (best_civilian_unit->is_busy() && !civilian_unit->is_busy())
		) {
			best_civilian_unit = civilian_unit;
		}
	}

	if (best_civilian_unit != nullptr) {
		best_civilian_unit->disband(false);
		this->change_population_growth(1);
		return;
	}

	//disband military unit, if possible
	for (auto it = this->military_units.rbegin(); it != this->military_units.rend(); ++it) {
		military_unit *military_unit = it->get();

		if (military_unit->get_character() != nullptr) {
			//character military units do not cost food, so disbanding them does nothing to help with starvation
			continue;
		}

		military_unit->disband(false);
		this->change_population_growth(1);
		return;
	}

	assert_throw(false);
}

population_unit *country_game_data::choose_starvation_population_unit()
{
	std::vector<population_unit *> population_units;

	for (population_unit *population_unit : this->get_population_units()) {
		if (population_unit->get_settlement()->get_game_data()->get_population_unit_count() == 1) {
			//do not remove a settlement's last population unit
			continue;
		}

		if (
			population_units.empty()
			|| population_units.at(0)->get_type()->get_output_value() > population_unit->get_type()->get_output_value()
		) {
			population_units.clear();
			population_units.push_back(population_unit);
		} else if (population_units.at(0)->get_type()->get_output_value() == population_unit->get_type()->get_output_value()) {
			population_units.push_back(population_unit);
		}
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
		for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
			if (!settlement->get_game_data()->is_built()) {
				continue;
			}

			food_consumption -= settlement->get_game_data()->get_free_food_consumption();
		}
	}

	return food_consumption;
}

QVariantList country_game_data::get_building_slots_qvariant_list() const
{
	std::vector<const country_building_slot *> available_building_slots;

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		if (!building_slot->is_available()) {
			continue;
		}

		available_building_slots.push_back(building_slot.get());
	}

	return container::to_qvariant_list(available_building_slots);
}

void country_game_data::initialize_building_slots()
{
	//initialize building slots, placing them in random order
	std::vector<building_slot_type *> building_slot_types = building_slot_type::get_all();
	vector::shuffle(building_slot_types);

	for (const building_slot_type *building_slot_type : building_slot_types) {
		this->building_slots.push_back(make_qunique<country_building_slot>(building_slot_type, this->country));
		this->building_slot_map[building_slot_type] = this->building_slots.back().get();
	}
}

const building_type *country_game_data::get_slot_building(const building_slot_type *slot_type) const
{
	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		return find_iterator->second->get_building();
	}

	assert_throw(false);

	return nullptr;
}

void country_game_data::set_slot_building(const building_slot_type *slot_type, const building_type *building)
{
	if (building != nullptr) {
		assert_throw(building->get_slot_type() == slot_type);
	}

	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		find_iterator->second->set_building(building);
		return;
	}

	assert_throw(false);
}

bool country_game_data::has_building(const building_type *building) const
{
	return this->get_slot_building(building->get_slot_type()) == building;
}

void country_game_data::clear_buildings()
{
	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		building_slot->set_building(nullptr);
	}
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

	country_building_slot *country_building_slot = this->get_building_slot(building->get_slot_type());

	assert_throw(country_building_slot != nullptr);

	if (count == 0) {
		//lost last settlement building
		if (country_building_slot->get_building() == building) {
			//get the best settlement building to replace the one that was lost (if any), and set it to the building slot

			const building_type *best_building = nullptr;
			int best_level = 0;

			for (const auto &[settlement_building, building_count] : this->settlement_building_counts) {
				if (settlement_building->get_slot_type() != country_building_slot->get_type()) {
					continue;
				}

				const int level = settlement_building->get_level();
				if (level > best_level) {
					best_building = settlement_building;
					best_level = level;
				}
			}

			country_building_slot->set_building(best_building);
		}
	} else if (count > 0 && old_count == 0) {
		//gained first settlement building
		if (country_building_slot->can_gain_building(building)) {
			country_building_slot->set_building(building);
		}
	}

	if (building->get_country_modifier() != nullptr && this->get_settlement_count() != 0) {
		//reapply the settlement building's country modifier with the updated count
		building->get_country_modifier()->apply(this->country, centesimal_int(-old_count) / this->get_settlement_count());
		building->get_country_modifier()->apply(this->country, centesimal_int(count) / this->get_settlement_count());
	}

	if (building->get_stackable_country_modifier() != nullptr) {
		building->get_stackable_country_modifier()->apply(this->country, change);
	}

	if (game::get()->is_running()) {
		emit settlement_building_counts_changed();
	}
}

void country_game_data::set_inflation(const centesimal_int &inflation)
{
	if (inflation == this->get_inflation()) {
		return;
	}

	if (inflation < 0) {
		this->set_inflation(centesimal_int(0));
		return;
	}

	if (!this->country->is_great_power()) {
		//minor nations cannot be affected by inflation
		this->set_inflation(centesimal_int(0));
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_input_wealth() == 0) {
				continue;
			}

			const int input_wealth = building_slot->get_production_type_input_wealth(production_type);
			this->change_wealth(input_wealth);
			this->change_wealth_income(input_wealth);
		}
	}

	this->inflation = inflation;

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_input_wealth() == 0) {
				continue;
			}

			const int input_wealth = building_slot->get_production_type_input_wealth(production_type);
			this->change_wealth(-input_wealth);
			this->change_wealth_income(-input_wealth);
		}
	}

	emit inflation_changed();
}

QVariantList country_game_data::get_available_commodities_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_commodities());
}

QVariantList country_game_data::get_tradeable_commodities_qvariant_list() const
{
	std::vector<const commodity *> tradeable_commodities = container::to_vector(this->get_tradeable_commodities());

	std::sort(tradeable_commodities.begin(), tradeable_commodities.end(), [](const commodity *lhs, const commodity *rhs) {
		if (lhs->get_base_price() != rhs->get_base_price()) {
			return lhs->get_base_price() > rhs->get_base_price();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	return container::to_qvariant_list(tradeable_commodities);
}

QVariantList country_game_data::get_stored_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_stored_commodities());
}

void country_game_data::set_stored_commodity(const commodity *commodity, const int value)
{
	if (value == this->get_stored_commodity(commodity)) {
		return;
	}

	if (value < 0 && !commodity->is_negative_allowed()) {
		throw std::runtime_error("Tried to set the storage of commodity \"" + commodity->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\" to a negative number.");
	}

	if (commodity->is_convertible_to_wealth()) {
		assert_throw(value > 0);
		const int wealth_conversion_income = commodity->get_wealth_value() * value;
		this->change_wealth(wealth_conversion_income);
		this->country->get_turn_data()->add_income_transaction(income_transaction_type::liquidated_riches, wealth_conversion_income, commodity, value);
		return;
	}

	if (value > this->get_storage_capacity() && !commodity->is_abstract()) {
		this->set_stored_commodity(commodity, this->get_storage_capacity());
		return;
	}

	if (commodity == defines::get()->get_prestige_commodity()) {
		this->change_score(-this->get_stored_commodity(commodity));
	}

	if (value <= 0) {
		this->stored_commodities.erase(commodity);
	} else {
		this->stored_commodities[commodity] = value;
	}

	if (commodity == defines::get()->get_prestige_commodity()) {
		this->change_score(value);
	}

	if (this->get_offer(commodity) > value) {
		this->set_offer(commodity, value);
	}

	if (game::get()->is_running()) {
		emit stored_commodities_changed();
	}
}

int country_game_data::get_stored_food() const
{
	int stored_food = 0;

	for (const auto &[commodity, quantity] : this->get_stored_commodities()) {
		if (commodity->is_food()) {
			stored_food += quantity;
		}
	}

	return stored_food;
}

void country_game_data::set_storage_capacity(const int capacity)
{
	if (capacity == this->get_storage_capacity()) {
		return;
	}

	this->storage_capacity = capacity;

	if (game::get()->is_running()) {
		emit storage_capacity_changed();
	}
}

QVariantList country_game_data::get_commodity_inputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_inputs());
}

int country_game_data::get_commodity_input(const QString &commodity_identifier) const
{
	return this->get_commodity_input(commodity::get(commodity_identifier.toStdString()));
}

void country_game_data::change_commodity_input(const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->commodity_inputs[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_inputs.erase(commodity);
	}

	if (commodity->get_base_price() != 0) {
		this->change_score(-change * commodity->get_base_price());
	}

	if (game::get()->is_running()) {
		emit commodity_inputs_changed();
	}
}

QVariantList country_game_data::get_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_outputs());
}

int country_game_data::get_commodity_output(const QString &commodity_identifier) const
{
	return this->get_commodity_output(commodity::get(commodity_identifier.toStdString()));
}

void country_game_data::change_commodity_output(const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->commodity_outputs[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_outputs.erase(commodity);
	}

	if (commodity->get_base_price() != 0 || commodity->get_wealth_value() != 0) {
		const int commodity_value = commodity->get_base_price() != 0 ? commodity->get_base_price() : commodity->get_wealth_value();
		this->change_score(change * commodity_value);
	}

	if (game::get()->is_running()) {
		emit commodity_outputs_changed();
	}

	if (change < 0 && !commodity->is_storable() && !commodity->is_negative_allowed()) {
		//decrease consumption of non-storable commodities immediately if the net output goes below zero, since for those commodities consumption cannot be fulfilled by storage
		while (this->get_net_commodity_output(commodity) < 0) {
			this->decrease_commodity_consumption(commodity);
		}
	}
}

void country_game_data::calculate_settlement_commodity_outputs()
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->calculate_settlement_commodity_outputs();
	}
}

void country_game_data::calculate_settlement_commodity_output(const commodity *commodity)
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->calculate_settlement_commodity_output(commodity);
	}
}

QVariantList country_game_data::get_commodity_consumptions_qvariant_list() const
{
	commodity_map<int> int_commodity_consumptions;

	for (const auto &[commodity, consumption] : this->get_commodity_consumptions()) {
		int_commodity_consumptions[commodity] = consumption.to_int();
	}

	return archimedes::map::to_qvariant_list(int_commodity_consumptions);
}

int country_game_data::get_commodity_consumption(const QString &commodity_identifier) const
{
	return this->get_commodity_consumption(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_game_data::change_commodity_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->commodity_consumptions[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_consumptions.erase(commodity);
	}

	if (game::get()->is_running()) {
		emit commodity_consumptions_changed();
	}
}

void country_game_data::change_commodity_demand(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->commodity_demands[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_demands.erase(commodity);
	}
}

void country_game_data::assign_production()
{
	bool changed = true;

	while (changed) {
		changed = false;

		for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
			const building_type *building_type = building_slot->get_building();

			if (building_type == nullptr) {
				continue;
			}

			for (const production_type *production_type : building_slot->get_available_production_types()) {
				if (!building_slot->can_increase_production(production_type)) {
					continue;
				}

				building_slot->increase_production(production_type);
				changed = true;
			}
		}
	}
}

void country_game_data::decrease_wealth_consumption(const bool restore_inputs)
{
	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		const building_type *building_type = building_slot->get_building();

		if (building_type == nullptr) {
			continue;
		}

		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_input_wealth() == 0) {
				continue;
			}

			if (!building_slot->can_decrease_production(production_type)) {
				continue;
			}

			building_slot->decrease_production(production_type, restore_inputs);
			return;
		}
	}

	assert_throw(false);
}

void country_game_data::decrease_commodity_consumption(const commodity *commodity, const bool restore_inputs)
{
	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		const building_type *building_type = building_slot->get_building();

		if (building_type == nullptr) {
			continue;
		}

		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (!production_type->get_input_commodities().contains(commodity)) {
				continue;
			}

			if (!building_slot->can_decrease_production(production_type)) {
				continue;
			}

			building_slot->decrease_production(production_type, restore_inputs);
			return;
		}
	}

	assert_throw(false);
}

bool country_game_data::produces_commodity(const commodity *commodity) const
{
	if (this->get_commodity_output(commodity) > 0) {
		return true;
	}

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->produces_commodity(commodity)) {
			return true;
		}
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() == commodity && building_slot->get_production_type_output(production_type) > 0) {
				return true;
			}
		}
	}

	return false;
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

QVariantList country_game_data::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

void country_game_data::add_technology(const technology *technology)
{
	if (this->has_technology(technology)) {
		return;
	}

	this->technologies.insert(technology);

	if (technology->get_modifier() != nullptr) {
		technology->get_modifier()->apply(this->country, 1);
	}

	for (const commodity *enabled_commodity : technology->get_enabled_commodities()) {
		this->add_available_commodity(enabled_commodity);

		if (enabled_commodity->is_tradeable()) {
			this->add_tradeable_commodity(enabled_commodity);
		}
	}

	for (const resource *discovered_resource : technology->get_enabled_resources()) {
		for (const province *province : this->get_provinces()) {
			const province_game_data *province_game_data = province->get_game_data();

			if (!province_game_data->get_resource_counts().contains(discovered_resource)) {
				continue;
			}

			for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
				const tile *tile = map::get()->get_tile(tile_pos);
				const resource *tile_resource = tile->get_resource();

				if (tile_resource != discovered_resource) {
					continue;
				}

				if (!tile->is_resource_discovered()) {
					map::get()->set_tile_resource_discovered(tile_pos, true);
				}
			}
		}
	}

	if (game::get()->is_running()) {
		emit technologies_changed();
	}
}

void country_game_data::add_technology_with_prerequisites(const technology *technology)
{
	this->add_technology(technology);

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		this->add_technology_with_prerequisites(prerequisite);
	}
}

std::vector<const technology *> country_game_data::get_available_technologies() const
{
	std::vector<const technology *> available_technologies;

	for (const technology *technology : technology::get_all()) {
		if (!this->is_technology_available(technology)) {
			continue;
		}

		available_technologies.push_back(technology);
	}

	std::sort(available_technologies.begin(), available_technologies.end(), technology_compare());

	return available_technologies;
}

QVariantList country_game_data::get_available_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_technologies());
}

bool country_game_data::is_technology_available(const technology *technology) const
{
	if (technology->is_discovery()) {
		return false;
	}

	if (this->has_technology(technology)) {
		return false;
	}

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		if (!this->has_technology(prerequisite)) {
			return false;
		}
	}

	return true;
}

QVariantList country_game_data::get_future_technologies_qvariant_list() const
{
	std::vector<technology *> future_technologies = technology::get_all();
	std::erase_if(future_technologies, [this](const technology *technology) {
		if (this->has_technology(technology)) {
			return true;
		}

		bool has_all_prerequisites = true;
		for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
			if (!this->has_technology(prerequisite)) {
				has_all_prerequisites = false;
			}
		}
		if (has_all_prerequisites) {
			return true;
		}

		return false;
	});

	std::sort(future_technologies.begin(), future_technologies.end(), technology_compare());

	return container::to_qvariant_list(future_technologies);
}

void country_game_data::set_current_research(const technology *technology)
{
	if (technology == this->get_current_research()) {
		return;
	}

	this->current_research = technology;

	emit current_research_changed();
}

void country_game_data::choose_current_research()
{
	const std::map<technology_category, const technology *> research_choice_map = this->get_research_choice_map();

	if (research_choice_map.empty()) {
		return;
	}

	if (this->is_ai()) {
		const technology *chosen_technology = this->get_ai_research_choice(research_choice_map);
		this->set_current_research(chosen_technology);
	} else {
		const std::vector<const technology *> potential_technologies = archimedes::map::get_values(research_choice_map);
		emit engine_interface::get()->current_research_choosable(container::to_qvariant_list(potential_technologies));
	}
}

void country_game_data::on_technology_researched(const technology *technology)
{
	if (technology == this->get_current_research()) {
		this->set_current_research(nullptr);
	}

	this->add_technology(technology);

	if (technology->grants_free_technology()) {
		bool first_to_research = true;

		//technology grants a free technology for the first one to research it
		for (const metternich::country *country : game::get()->get_countries()) {
			if (country == this->country) {
				continue;
			}

			if (country->get_game_data()->has_technology(technology)) {
				first_to_research = false;
				break;
			}
		}

		if (first_to_research) {
			this->gain_free_technologies(1);
		}
	}

	emit technology_researched(technology);
}

std::map<technology_category, const technology *> country_game_data::get_research_choice_map() const
{
	const std::vector<const technology *> available_technologies = this->get_available_technologies();

	if (available_technologies.empty()) {
		return {};
	}

	std::map<technology_category, std::vector<const technology *>> potential_technologies_per_category;

	for (const technology *technology : available_technologies) {
		std::vector<const metternich::technology *> &category_technologies = potential_technologies_per_category[technology->get_category()];

		const int weight = 1;
		for (int i = 0; i < weight; ++i) {
			category_technologies.push_back(technology);
		}
	}

	assert_throw(!potential_technologies_per_category.empty());

	std::map<technology_category, const technology *> research_choice_map;
	const std::vector<technology_category> potential_categories = archimedes::map::get_keys(potential_technologies_per_category);

	for (const technology_category category : potential_categories) {
		research_choice_map[category] = vector::get_random(potential_technologies_per_category[category]);
	}

	return research_choice_map;
}

const technology *country_game_data::get_ai_research_choice(const std::map<technology_category, const technology *> &research_choice_map) const
{
	assert_throw(this->is_ai());

	std::vector<const technology *> preferred_technologies;

	int best_desire = 0;
	for (const auto &[category, technology] : research_choice_map) {
		int desire = 100 / (technology->get_total_prerequisite_depth() + 1);

		for (const journal_entry *journal_entry : this->get_active_journal_entries()) {
			if (vector::contains(journal_entry->get_researched_technologies(), technology)) {
				desire += journal_entry::ai_technology_desire_modifier;
			}
		}

		assert_throw(desire > 0);

		if (desire > best_desire) {
			preferred_technologies.clear();
			best_desire = desire;
		}

		if (desire >= best_desire) {
			preferred_technologies.push_back(technology);
		}
	}

	assert_throw(!preferred_technologies.empty());

	const technology *chosen_technology = vector::get_random(preferred_technologies);
	return chosen_technology;
}

void country_game_data::gain_free_technology()
{
	const std::map<technology_category, const technology *> research_choice_map = this->get_research_choice_map();

	if (research_choice_map.empty()) {
		return;
	}

	if (this->is_ai()) {
		const technology *chosen_technology = this->get_ai_research_choice(research_choice_map);
		this->gain_free_technology(chosen_technology);
	} else {
		if (this->get_current_research() != nullptr) {
			this->gain_free_technology(this->get_current_research());
		} else {
			const std::vector<const technology *> potential_technologies = archimedes::map::get_values(research_choice_map);
			emit engine_interface::get()->free_technology_choosable(container::to_qvariant_list(potential_technologies));
		}
	}
}

void country_game_data::gain_free_technologies(const int count)
{
	assert_throw(count > 0);

	this->free_technology_count += count;
	this->gain_free_technology();
}

void country_game_data::gain_technologies_known_by_others()
{
	static constexpr int min_countries = 2;

	technology_map<int> technology_known_counts;

	for (const metternich::country *known_country : this->get_known_countries()) {
		for (const technology *technology : known_country->get_game_data()->get_technologies()) {
			++technology_known_counts[technology];
		}
	}

	for (const auto &[technology, known_count] : technology_known_counts) {
		if (known_count < min_countries) {
			continue;
		}

		if (this->has_technology(technology)) {
			continue;
		}

		this->on_technology_researched(technology);
	}
}

void country_game_data::set_government_type(const metternich::government_type *government_type)
{
	if (government_type == this->get_government_type()) {
		return;
	}

	if (this->get_government_type() != nullptr) {
		this->get_government_type()->get_modifier()->remove(this->country);
	}

	this->government_type = government_type;

	if (this->get_government_type() != nullptr) {
		this->get_government_type()->get_modifier()->apply(this->country);
	}

	if (game::get()->is_running()) {
		emit government_type_changed();
	}
}

QVariantList country_game_data::get_policy_values_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_policy_values());
}

void country_game_data::set_policy_value(const policy *policy, const int value)
{
	if (value == this->get_policy_value(policy)) {
		return;
	}

	if (value < policy::min_value) {
		this->set_policy_value(policy, policy::min_value);
		return;
	} else if (value > policy::max_value) {
		this->set_policy_value(policy, policy::max_value);
		return;
	}

	policy->apply_modifier(this->country, value, -1);

	if (value == 0) {
		this->policy_values.erase(policy);
	} else {
		this->policy_values[policy] = value;
	}

	policy->apply_modifier(this->country, value, 1);

	if (game::get()->is_running()) {
		emit policy_values_changed();
	}
}

bool country_game_data::can_change_policy_value(const metternich::policy *policy, const int change) const
{
	const int new_value = this->get_policy_value(policy) + change;
	if (new_value < policy::min_value) {
		return false;
	} else if (new_value > policy::max_value) {
		return false;
	}

	for (const auto &[commodity, cost] : policy->get_change_commodity_costs()) {
		if (this->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	return true;
}

void country_game_data::do_policy_value_change(const policy *policy, const int change)
{
	for (const auto &[commodity, cost] : policy->get_change_commodity_costs()) {
		this->change_stored_commodity(commodity, -cost);
	}

	this->change_policy_value(policy, change);
}

void country_game_data::check_characters()
{
	this->check_ruler();

	if (game::get()->get_rules()->are_advisors_enabled() && this->can_have_advisors()) {
		this->check_advisors();
	}

	this->check_leaders();
}

void country_game_data::set_ruler(const character *ruler)
{
	if (ruler == this->get_ruler()) {
		return;
	}

	const character *old_ruler = this->get_ruler();

	if (old_ruler != nullptr) {
		for (const trait *trait : old_ruler->get_traits()) {
			if (trait->get_ruler_modifier() != nullptr) {
				trait->get_ruler_modifier()->remove(this->country);
			}
		}

		old_ruler->get_game_data()->set_country(nullptr);
	}

	this->ruler = ruler;

	if (this->get_ruler() != nullptr) {
		for (const trait *trait : this->get_ruler()->get_traits()) {
			if (trait->get_ruler_modifier() != nullptr) {
				trait->get_ruler_modifier()->apply(this->country);
			}
		}

		this->get_ruler()->get_game_data()->set_country(this->country);
	}

	if (game::get()->is_running()) {
		emit ruler_changed();

		if (old_ruler != nullptr) {
			emit old_ruler->get_game_data()->ruler_changed();
		}

		if (ruler != nullptr) {
			emit ruler->get_game_data()->ruler_changed();
		}
	}
}

void country_game_data::check_ruler()
{
	//remove the ruler if they have become obsolete
	if (this->get_ruler() != nullptr && this->get_ruler()->get_obsolescence_technology() != nullptr && this->has_technology(this->get_ruler()->get_obsolescence_technology())) {
		if (game::get()->is_running()) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Ruler Died", interior_minister_portrait, std::format("Our ruler, {}, has died!", this->get_ruler()->get_full_name()));
			}

			context ctx(this->country);
			ctx.source_scope = this->get_ruler();
			country_event::check_events_for_scope(this->country, event_trigger::ruler_death, ctx);
		}

		this->set_ruler(nullptr);
		this->get_ruler()->get_game_data()->set_dead(true);
	}

	//if the country has no ruler, see if there is any character which can become its ruler
	if (this->get_ruler() == nullptr) {
		std::vector<const character *> potential_rulers;

		for (const character *character : this->country->get_rulers()) {
			assert_throw(character->is_ruler());

			const character_game_data *character_game_data = character->get_game_data();
			if (character_game_data->get_country() != nullptr) {
				continue;
			}

			if (character_game_data->is_dead()) {
				continue;
			}

			if (character->get_required_technology() != nullptr && !this->has_technology(character->get_required_technology())) {
				continue;
			}

			if (character->get_obsolescence_technology() != nullptr && this->has_technology(character->get_obsolescence_technology())) {
				continue;
			}

			if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->country, read_only_context(this->country))) {
				continue;
			}

			potential_rulers.push_back(character);
		}

		if (!potential_rulers.empty()) {
			this->set_ruler(vector::get_random(potential_rulers));

			if (this->country == game::get()->get_player_country() && game::get()->is_running()) {
				const portrait *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("New Ruler", interior_minister_portrait, std::format("{} has become our new ruler!\n\n{}", this->get_ruler()->get_full_name(), this->get_ruler()->get_ruler_modifier_string(this->country)));
			}
		}
	}
}

QVariantList country_game_data::get_advisors_qvariant_list() const
{
	return container::to_qvariant_list(this->get_advisors());
}

void country_game_data::check_advisors()
{
	//remove obsolete advisors
	const std::vector<const character *> advisors = this->get_advisors();
	for (const character *advisor : advisors) {
		if (advisor->get_obsolescence_technology() != nullptr && this->has_technology(advisor->get_obsolescence_technology())) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Retired", interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the advisor {} has decided to retire.", advisor->get_full_name()));
			}

			this->remove_advisor(advisor);
			advisor->get_game_data()->set_dead(true);
		}
	}

	if (this->is_under_anarchy()) {
		if (this->get_next_advisor() != nullptr) {
			this->set_next_advisor(nullptr);
		}

		return;
	}

	if (this->get_next_advisor() != nullptr) {
		if (this->get_next_advisor()->get_game_data()->get_country() != nullptr) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Unavailable", interior_minister_portrait, std::format("Your Excellency, the advisor {} has unfortunately decided to join {}, and is no longer available for recruitment.", this->get_next_advisor()->get_full_name(), this->get_next_advisor()->get_game_data()->get_country()->get_name()));
			}

			this->set_next_advisor(nullptr);
		} else if (this->get_next_advisor()->get_obsolescence_technology() != nullptr && this->has_technology(this->get_next_advisor()->get_obsolescence_technology())) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Unavailable", interior_minister_portrait, std::format("Your Excellency, the advisor {} is no longer available for recruitment.", this->get_next_advisor()->get_full_name()));
			}

			this->set_next_advisor(nullptr);
		} else {
			if (this->get_stored_commodity(defines::get()->get_advisor_commodity()) >= this->get_advisor_cost()) {
				this->change_stored_commodity(defines::get()->get_advisor_commodity(), -this->get_advisor_cost());

				this->add_advisor(this->get_next_advisor());

				if (this->get_next_advisor()->get_advisor_effects() != nullptr) {
					context ctx(country);
					this->get_next_advisor()->get_advisor_effects()->do_effects(country, ctx);
				}

				emit advisor_recruited(this->get_next_advisor());

				this->set_next_advisor(nullptr);
			}
		}
	} else {
		if (this->get_commodity_output(defines::get()->get_advisor_commodity()) > 0 || this->get_stored_commodity(defines::get()->get_advisor_commodity()) > 0) {
			this->choose_next_advisor();
		}
	}
}

void country_game_data::add_advisor(const character *advisor)
{
	assert_throw(game::get()->get_rules()->are_advisors_enabled());
	assert_throw(this->can_have_advisors());

	this->advisors.push_back(advisor);
	advisor->get_game_data()->set_country(this->country);
	advisor->apply_advisor_modifier(this->country, 1);

	emit advisors_changed();
}

void country_game_data::remove_advisor(const character *advisor)
{
	assert_throw(advisor->get_game_data()->get_country() == this->country);

	std::erase(this->advisors, advisor);
	advisor->get_game_data()->set_country(nullptr);
	advisor->apply_advisor_modifier(this->country, -1);

	emit advisors_changed();
}

void country_game_data::clear_advisors()
{
	const std::vector<const character *> advisors = this->get_advisors();
	for (const character *advisor : advisors) {
		this->remove_advisor(advisor);
	}

	assert_throw(this->get_advisors().empty());

	emit advisors_changed();
}

void country_game_data::choose_next_advisor()
{
	std::map<advisor_category, std::vector<const character *>> potential_advisors_per_category;

	for (const character *character : character::get_all()) {
		if (!character->is_advisor()) {
			continue;
		}

		const character_game_data *character_game_data = character->get_game_data();
		if (character_game_data->get_country() != nullptr) {
			continue;
		}

		if (character_game_data->is_dead()) {
			continue;
		}

		if (character->get_required_technology() != nullptr && !this->has_technology(character->get_required_technology())) {
			continue;
		}

		if (character->get_obsolescence_technology() != nullptr && this->has_technology(character->get_obsolescence_technology())) {
			continue;
		}

		if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->country, read_only_context(this->country))) {
			continue;
		}

		int weight = 1;

		if (character->get_home_settlement()->get_game_data()->get_owner() == this->country) {
			weight += 4;
		}

		std::vector<const metternich::character *> &category_advisors = potential_advisors_per_category[character->get_advisor_type()->get_category()];

		for (int i = 0; i < weight; ++i) {
			category_advisors.push_back(character);
		}
	}

	if (potential_advisors_per_category.empty()) {
		return;
	}

	std::map<advisor_category, const character *> potential_advisor_map;
	const std::vector<advisor_category> potential_categories = archimedes::map::get_keys(potential_advisors_per_category);

	for (const advisor_category category : potential_categories) {
		potential_advisor_map[category] = vector::get_random(potential_advisors_per_category[category]);
	}

	if (this->is_ai()) {
		std::vector<const character *> preferred_advisors;

		int best_desire = 0;
		for (const auto &[category, advisor] : potential_advisor_map) {
			//consider advisors with special modifiers or effects to have maximum skill, for the purposes of preferring to select them
			int desire = (advisor->get_skill() != 0 ? advisor->get_skill() : character::max_skill) * 100;

			for (const journal_entry *journal_entry : this->get_active_journal_entries()) {
				if (vector::contains(journal_entry->get_recruited_advisors(), advisor)) {
					desire += journal_entry::ai_advisor_desire_modifier;
				}
			}

			assert_throw(desire > 0);

			if (desire > best_desire) {
				preferred_advisors.clear();
				best_desire = desire;
			}

			if (desire >= best_desire) {
				preferred_advisors.push_back(advisor);
			}
		}

		assert_throw(!preferred_advisors.empty());

		const character *chosen_advisor = vector::get_random(preferred_advisors);
		this->set_next_advisor(chosen_advisor);
	} else {
		const std::vector<const character *> potential_advisors = archimedes::map::get_values(potential_advisor_map);
		emit engine_interface::get()->next_advisor_choosable(container::to_qvariant_list(potential_advisors));
	}
}

bool country_game_data::can_have_advisors() const
{
	//only great powers can have advisors
	return this->country->get_type() == country_type::great_power;
}

QVariantList country_game_data::get_leaders_qvariant_list() const
{
	return container::to_qvariant_list(this->get_leaders());
}

void country_game_data::check_leaders()
{
	//remove obsolete leaders
	const std::vector<const character *> leaders = this->get_leaders();
	for (const character *leader : leaders) {
		if (leader->get_obsolescence_technology() != nullptr && this->has_technology(leader->get_obsolescence_technology())) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *war_minister_portrait = defines::get()->get_war_minister_portrait();

				const std::string leader_type_name = leader->get_leader_type_name();

				engine_interface::get()->add_notification(std::format("{} Retired", leader_type_name), war_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the {} {} has decided to retire.", string::lowered(leader_type_name), leader->get_full_name()));
			}

			this->remove_leader(leader);
			leader->get_game_data()->set_dead(true);
		}
	}

	if (this->is_under_anarchy()) {
		if (this->get_next_leader() != nullptr) {
			this->set_next_leader(nullptr);
		}

		return;
	}

	if (this->get_next_leader() != nullptr) {
		if (this->get_next_leader()->get_game_data()->get_country() != nullptr) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *war_minister_portrait = defines::get()->get_war_minister_portrait();

				const std::string leader_type_name = this->get_next_leader()->get_leader_type_name();

				engine_interface::get()->add_notification(std::format("{} Unavailable", leader_type_name), war_minister_portrait, std::format("Your Excellency, the {} {} has unfortunately decided to join {}, and is no longer available for recruitment.", string::lowered(leader_type_name), this->get_next_leader()->get_full_name(), this->get_next_leader()->get_game_data()->get_country()->get_name()));
			}

			this->set_next_leader(nullptr);
		} else if (
			(this->get_next_leader()->get_obsolescence_technology() != nullptr && this->has_technology(this->get_next_leader()->get_obsolescence_technology()))
			|| this->get_best_military_unit_category_type(this->get_next_leader()->get_military_unit_category(), this->get_next_leader()->get_culture()) == nullptr
		) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *war_minister_portrait = defines::get()->get_war_minister_portrait();

				const std::string leader_type_name = this->get_next_leader()->get_leader_type_name();

				engine_interface::get()->add_notification(std::format("{} Unavailable", leader_type_name), war_minister_portrait, std::format("Your Excellency, the {} {} is no longer available for recruitment.", string::lowered(leader_type_name), this->get_next_leader()->get_full_name()));
			}

			this->set_next_leader(nullptr);
		} else {
			if (this->get_stored_commodity(defines::get()->get_leader_commodity()) >= this->get_leader_cost()) {
				this->change_stored_commodity(defines::get()->get_leader_commodity(), -this->get_leader_cost());

				this->add_leader(this->get_next_leader());
				this->get_next_leader()->get_game_data()->deploy_to_province(this->get_capital_province());

				emit leader_recruited(this->get_next_leader());

				this->set_next_leader(nullptr);
			}
		}
	} else {
		if (this->get_commodity_output(defines::get()->get_leader_commodity()) > 0 || this->get_stored_commodity(defines::get()->get_leader_commodity()) > 0) {
			this->choose_next_leader();
		}
	}
}

void country_game_data::add_leader(const character *leader)
{
	this->leaders.push_back(leader);
	leader->get_game_data()->set_country(this->country);

	emit leaders_changed();
}

void country_game_data::remove_leader(const character *leader)
{
	assert_throw(leader->get_game_data()->get_country() == this->country);

	std::erase(this->leaders, leader);
	leader->get_game_data()->set_country(nullptr);

	emit leaders_changed();
}

void country_game_data::clear_leaders()
{
	const std::vector<const character *> leaders = this->get_leaders();
	for (const character *leader : leaders) {
		this->remove_leader(leader);
	}

	assert_throw(this->get_leaders().empty());

	emit leaders_changed();
}

void country_game_data::choose_next_leader()
{
	std::map<military_unit_category, std::vector<const character *>> potential_leaders_per_category;

	for (const character *character : character::get_all()) {
		if (!character->is_leader()) {
			continue;
		}

		const character_game_data *character_game_data = character->get_game_data();
		if (character_game_data->get_country() != nullptr) {
			continue;
		}

		if (character_game_data->is_dead()) {
			continue;
		}

		const military_unit_type *military_unit_type = this->get_best_military_unit_category_type(character->get_military_unit_category(), character->get_culture());
		if (military_unit_type == nullptr) {
			continue;
		}

		if (character->get_required_technology() != nullptr && !this->has_technology(character->get_required_technology())) {
			continue;
		}

		if (character->get_obsolescence_technology() != nullptr && this->has_technology(character->get_obsolescence_technology())) {
			continue;
		}

		if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->country, read_only_context(this->country))) {
			continue;
		}

		military_unit_category leader_category = character->get_military_unit_category();
		if (character->is_admiral()) {
			//place all admirals in the same category, even if they have different kinds of ships
			leader_category = military_unit_category::heavy_warship;
		}

		std::vector<const metternich::character *> &category_leaders = potential_leaders_per_category[leader_category];

		category_leaders.push_back(character);
	}

	if (potential_leaders_per_category.empty()) {
		return;
	}

	std::map<military_unit_category, const character *> potential_leader_map;
	const std::vector<military_unit_category> potential_categories = archimedes::map::get_keys(potential_leaders_per_category);

	for (const military_unit_category category : potential_categories) {
		potential_leader_map[category] = vector::get_random(potential_leaders_per_category[category]);
	}

	if (this->is_ai()) {
		std::vector<const character *> preferred_leaders;

		int best_desire = 0;
		for (const auto &[category, leader] : potential_leader_map) {
			const military_unit_type *military_unit_type = this->get_best_military_unit_category_type(leader->get_military_unit_category(), leader->get_culture());
			assert_throw(military_unit_type != nullptr);
			const int leader_score = military_unit_type->get_score();

			assert_throw(leader_score > 0);

			int desire = leader_score;

			for (const journal_entry *journal_entry : this->get_active_journal_entries()) {
				if (vector::contains(journal_entry->get_recruited_leaders(), leader)) {
					desire += journal_entry::ai_leader_desire_modifier;
				}
			}

			assert_throw(desire > 0);

			if (desire > best_desire) {
				preferred_leaders.clear();
				best_desire = desire;
			}

			if (desire >= best_desire) {
				preferred_leaders.push_back(leader);
			}
		}

		assert_throw(!preferred_leaders.empty());

		const character *chosen_leader = vector::get_random(preferred_leaders);
		this->set_next_leader(chosen_leader);
	} else {
		const std::vector<const character *> potential_leaders = archimedes::map::get_values(potential_leader_map);
		emit engine_interface::get()->next_leader_choosable(container::to_qvariant_list(potential_leaders));
	}
}

const military_unit_type *country_game_data::get_next_leader_military_unit_type() const
{
	if (this->get_next_leader() == nullptr) {
		return nullptr;
	}

	return this->get_best_military_unit_category_type(this->get_next_leader()->get_military_unit_category(), this->get_next_leader()->get_culture());
}

QVariantList country_game_data::get_bids_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_bids());
}

void country_game_data::set_bid(const commodity *commodity, const int value)
{
	if (value == this->get_bid(commodity)) {
		return;
	}

	if (value == 0) {
		this->bids.erase(commodity);
	} else {
		this->set_offer(commodity, 0);
		this->bids[commodity] = value;
	}

	if (game::get()->is_running()) {
		emit bids_changed();
	}
}

QVariantList country_game_data::get_offers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_offers());
}

void country_game_data::set_offer(const commodity *commodity, const int value)
{
	if (value == this->get_offer(commodity)) {
		return;
	}

	if (value > this->get_stored_commodity(commodity)) {
		this->set_offer(commodity, this->get_stored_commodity(commodity));
		return;
	}

	if (value == 0) {
		this->offers.erase(commodity);
	} else {
		this->set_bid(commodity, 0);
		this->offers[commodity] = value;
	}

	if (game::get()->is_running()) {
		emit offers_changed();
	}
}

void country_game_data::calculate_commodity_needs()
{
	this->commodity_needs.clear();

	if (!this->country->is_great_power()) {
		for (const auto &[commodity, demand] : this->get_commodity_demands()) {
			this->commodity_needs[commodity] += demand.to_int();
		}
	}
}

void country_game_data::assign_trade_orders()
{
	assert_throw(this->is_ai());

	this->bids.clear();
	this->offers.clear();

	if (this->is_under_anarchy()) {
		return;
	}

	for (const auto &[commodity, value] : this->get_stored_commodities()) {
		if (!this->can_trade_commodity(commodity)) {
			continue;
		}

		const int need = this->get_commodity_need(commodity);

		if (value > need) {
			this->set_offer(commodity, value);
		}
	}

	for (const auto &[commodity, demand] : this->get_commodity_demands()) {
		if (!this->can_trade_commodity(commodity)) {
			continue;
		}

		if (this->get_offer(commodity) != 0) {
			continue;
		}

		//increase demand if prices are lower than the base price, or the inverse if they are higher
		const centesimal_int effective_demand = demand * commodity->get_base_price() / game::get()->get_price(commodity);

		if (effective_demand == 0) {
			continue;
		}

		this->set_bid(commodity, effective_demand.to_int());
	}
}

void country_game_data::add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit)
{
	this->change_food_consumption(1);
	this->civilian_units.push_back(std::move(civilian_unit));
}

void country_game_data::remove_civilian_unit(civilian_unit *civilian_unit)
{
	this->change_food_consumption(-1);

	for (size_t i = 0; i < this->civilian_units.size(); ++i) {
		if (this->civilian_units[i].get() == civilian_unit) {
			this->civilian_units.erase(this->civilian_units.begin() + i);
			return;
		}
	}
}

void country_game_data::add_military_unit(qunique_ptr<military_unit> &&military_unit)
{
	if (military_unit->get_character() == nullptr) {
		this->change_food_consumption(1);
	}

	this->military_units.push_back(std::move(military_unit));
}

void country_game_data::remove_military_unit(military_unit *military_unit)
{
	if (military_unit->get_character() == nullptr) {
		this->change_food_consumption(-1);
	}

	for (size_t i = 0; i < this->military_units.size(); ++i) {
		if (this->military_units[i].get() == military_unit) {
			this->military_units.erase(this->military_units.begin() + i);
			return;
		}
	}
}

const military_unit_type *country_game_data::get_best_military_unit_category_type(const military_unit_category category, const culture *culture) const
{
	const military_unit_type *best_type = nullptr;
	int best_score = 0;

	for (const military_unit_class *military_unit_class : military_unit_class::get_all()) {
		if (military_unit_class->get_category() != category) {
			continue;
		}

		const military_unit_type *type = culture->get_military_class_unit_type(military_unit_class);

		if (type == nullptr) {
			continue;
		}

		if (type->get_required_technology() != nullptr && !this->has_technology(type->get_required_technology())) {
			continue;
		}

		const int score = type->get_score();

		if (score > best_score) {
			best_type = type;
		}
	}

	return best_type;
}

const military_unit_type *country_game_data::get_best_military_unit_category_type(const military_unit_category category) const
{
	return this->get_best_military_unit_category_type(category, this->country->get_culture());
}

void country_game_data::set_output_modifier(const int value)
{
	if (value == this->get_output_modifier()) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			this->change_commodity_output(production_type->get_output_commodity(), -building_slot->get_production_type_output(production_type));
		}
	}

	this->output_modifier = value;

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			this->change_commodity_output(production_type->get_output_commodity(), building_slot->get_production_type_output(production_type));
		}
	}

	this->calculate_settlement_commodity_outputs();

	if (game::get()->is_running()) {
		emit output_modifier_changed();
	}
}

void country_game_data::set_resource_output_modifier(const int value)
{
	if (value == this->get_resource_output_modifier()) {
		return;
	}

	this->resource_output_modifier = value;

	this->calculate_settlement_commodity_outputs();

	if (game::get()->is_running()) {
		emit resource_output_modifier_changed();
	}
}

void country_game_data::set_industrial_output_modifier(const int value)
{
	if (value == this->get_industrial_output_modifier()) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (!production_type->is_industrial()) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), -building_slot->get_production_type_output(production_type));
		}
	}

	this->industrial_output_modifier = value;

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (!production_type->is_industrial()) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), building_slot->get_production_type_output(production_type));
		}
	}

	if (game::get()->is_running()) {
		emit industrial_output_modifier_changed();
	}
}

void country_game_data::set_commodity_output_modifier(const commodity *commodity, const int value)
{
	if (value == this->get_commodity_output_modifier(commodity)) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), -building_slot->get_production_type_output(production_type));
		}
	}

	if (value == 0) {
		this->commodity_output_modifiers.erase(commodity);
	} else {
		this->commodity_output_modifiers[commodity] = value;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), building_slot->get_production_type_output(production_type));
		}
	}

	this->calculate_settlement_commodity_output(commodity);
}

void country_game_data::set_throughput_modifier(const int value)
{
	if (value == this->get_throughput_modifier()) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				if (input_commodity->is_storable()) {
					this->change_stored_commodity(input_commodity, input_value);
				}
				this->change_commodity_input(input_commodity, -input_value);
			}
		}
	}

	this->throughput_modifier = value;

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				if (input_commodity->is_storable()) {
					this->change_stored_commodity(input_commodity, -input_value);
				}
				this->change_commodity_input(input_commodity, input_value);
			}
		}
	}

	if (game::get()->is_running()) {
		emit throughput_modifier_changed();
	}
}

void country_game_data::set_commodity_throughput_modifier(const commodity *commodity, const int value)
{
	if (value == this->get_commodity_throughput_modifier(commodity)) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				if (input_commodity->is_storable()) {
					this->change_stored_commodity(input_commodity, input_value);
				}
				this->change_commodity_input(input_commodity, -input_value);
			}
		}
	}

	if (value == 0) {
		this->commodity_throughput_modifiers.erase(commodity);
	} else {
		this->commodity_throughput_modifiers[commodity] = value;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				if (input_commodity->is_storable()) {
					this->change_stored_commodity(input_commodity, -input_value);
				}
				this->change_commodity_input(input_commodity, input_value);
			}
		}
	}
}

void country_game_data::change_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->improved_resource_commodity_bonuses[resource][commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->improved_resource_commodity_bonuses[resource].erase(commodity);

		if (this->improved_resource_commodity_bonuses[resource].empty()) {
			this->improved_resource_commodity_bonuses.erase(resource);
		}
	}

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->change_improved_resource_commodity_bonus(resource, commodity, change);
	}
}

void country_game_data::set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
{
	const int old_value = this->get_commodity_bonus_for_tile_threshold(commodity, threshold);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->commodity_bonuses_for_tile_thresholds[commodity].erase(threshold);

		if (this->commodity_bonuses_for_tile_thresholds[commodity].empty()) {
			this->commodity_bonuses_for_tile_thresholds.erase(commodity);
		}
	} else {
		this->commodity_bonuses_for_tile_thresholds[commodity][threshold] = value;
	}

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->change_commodity_bonus_for_tile_threshold(commodity, threshold, value - old_value);
	}
}

void country_game_data::change_capital_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->capital_commodity_bonuses_per_population[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->capital_commodity_bonuses_per_population.erase(commodity);
	}

	if (this->get_capital() != nullptr) {
		this->get_capital()->get_game_data()->calculate_commodity_outputs();
	}
}

void country_game_data::set_category_research_modifier(const technology_category category, const int value)
{
	if (value == this->get_category_research_modifier(category)) {
		return;
	}

	if (value == 0) {
		this->category_research_modifiers.erase(category);
	} else {
		this->category_research_modifiers[category] = value;
	}
}

void country_game_data::decrement_scripted_modifiers()
{
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
		if (!this->has_technology(tile_resource->get_discovery_technology())) {
			this->add_technology(tile_resource->get_discovery_technology());

			if (game::get()->is_running()) {
				emit technology_researched(tile_resource->get_discovery_technology());
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
			if (!this->has_technology(tile_resource->get_discovery_technology())) {
				this->add_technology(tile_resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit technology_researched(tile_resource->get_discovery_technology());
				}
			}
		}
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
		this->change_ai_building_desire_modifier(building, journal_entry::ai_building_desire_modifier);
	}

	for (const auto &[settlement, buildings] : journal_entry->get_built_settlement_buildings_with_requirements()) {
		for (const building_type *building : buildings) {
			this->change_ai_settlement_building_desire_modifier(settlement, building, journal_entry::ai_building_desire_modifier);
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
		this->change_ai_building_desire_modifier(building, -journal_entry::ai_building_desire_modifier);
	}

	for (const auto &[settlement, buildings] : journal_entry->get_built_settlement_buildings_with_requirements()) {
		for (const building_type *building : buildings) {
			this->change_ai_settlement_building_desire_modifier(settlement, building, -journal_entry::ai_building_desire_modifier);
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

void country_game_data::set_gain_technologies_known_by_others_count(const int value)
{
	const int old_value = this->get_gain_technologies_known_by_others_count();
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	this->gain_technologies_known_by_others_count = value;

	if (old_value == 0) {
		this->gain_technologies_known_by_others();
	}
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

void country_game_data::set_free_infantry_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_infantry_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_infantry_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_infantry_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_infantry()) {
				continue;
			}

			military_unit->check_free_promotions();
		}
	}
}

void country_game_data::set_free_cavalry_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_cavalry_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_cavalry_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_cavalry_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_cavalry()) {
				continue;
			}

			military_unit->check_free_promotions();
		}
	}
}

void country_game_data::set_free_artillery_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_artillery_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_artillery_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_artillery_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_artillery()) {
				continue;
			}

			military_unit->check_free_promotions();
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

}
