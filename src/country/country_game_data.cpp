#include "metternich.h"

#include "country/country_game_data.h"

#include "character/advisor_category.h"
#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "character/character_trait.h"
#include "character/character_trait_type.h"
#include "character/character_type.h"
#include "country/consulate.h"
#include "country/country.h"
#include "country/country_ai.h"
#include "country/country_rank.h"
#include "country/country_tier.h"
#include "country/country_tier_data.h"
#include "country/country_turn_data.h"
#include "country/country_type.h"
#include "country/culture.h"
#include "country/diplomacy_state.h"
#include "country/government_group.h"
#include "country/government_type.h"
#include "country/idea.h"
#include "country/idea_slot.h"
#include "country/idea_trait.h"
#include "country/idea_type.h"
#include "country/journal_entry.h"
#include "country/law.h"
#include "country/law_group.h"
#include "country/office.h"
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
#include "population/education_type.h"
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
#include "technology/technology.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/civilian_unit.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"
#include "unit/transporter.h"
#include "unit/transporter_class.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
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
	connect(this, &country_game_data::tier_changed, this, &country_game_data::title_name_changed);
	connect(this, &country_game_data::tier_changed, this, &country_game_data::office_title_names_changed);
	connect(this, &country_game_data::government_type_changed, this, &country_game_data::title_name_changed);
	connect(this, &country_game_data::government_type_changed, this, &country_game_data::office_title_names_changed);
	connect(this, &country_game_data::religion_changed, this, &country_game_data::title_name_changed);
	connect(this, &country_game_data::religion_changed, this, &country_game_data::office_title_names_changed);
	connect(this, &country_game_data::office_holders_changed, this, &country_game_data::office_title_names_changed);
	connect(this, &country_game_data::rank_changed, this, &country_game_data::type_name_changed);

	for (const commodity *commodity : commodity::get_all()) {
		if (!commodity->is_enabled()) {
			continue;
		}

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

		this->do_education();
		this->do_production();
		this->do_civilian_unit_recruitment();
		this->do_research();
		this->do_population_growth();
		this->do_everyday_consumption();
		this->do_luxury_consumption();
		this->do_construction();
		this->do_cultural_change();

		for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
			civilian_unit->do_turn();
		}

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			military_unit->do_turn();
		}

		for (const qunique_ptr<transporter> &transporter : this->transporters) {
			transporter->do_turn();
		}

		for (const qunique_ptr<army> &army : this->armies) {
			army->do_turn();
		}

		this->armies.clear();

		this->decrement_scripted_modifiers();

		this->check_journal_entries();

		this->check_government_type();
		this->check_characters();
		this->check_ideas();
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to process turn for country \"{}\".", this->country->get_identifier())));
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

			this->change_stored_commodity(commodity, output.to_int());
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
					const int output = this->get_commodity_output(commodity).to_int();
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

void country_game_data::do_education()
{
	try {
		//FIXME: add preference for education being automatically assigned for person players?
		if (this->is_ai()) {
			//this->assign_education();
		}

		for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
			for (const education_type *education_type : building_slot->get_available_education_types()) {
				const int output = building_slot->get_education_type_output(education_type);
				if (output == 0) {
					continue;
				}

				int remaining_output = output;

				while (remaining_output > 0) {
					population_unit *population_unit = this->choose_education_population_unit(education_type);
					if (population_unit == nullptr) {
						break;
					}
					population_unit->set_type(education_type->get_output_population_type());

					--remaining_output;
				}

				building_slot->change_education(education_type, -(building_slot->get_education_type_employed_capacity(education_type) - remaining_output), false);
				building_slot->change_education(education_type, -remaining_output, true);
				assert_throw(building_slot->get_education_type_employed_capacity(education_type) == 0);
			}
		}

		this->population_type_inputs.clear();
		this->population_type_outputs.clear();
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing education recruitment for country \"{}\".", this->country->get_identifier())));
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

void country_game_data::do_research()
{
	try {
		if (this->get_gain_technologies_known_by_others_count() > 0) {
			this->gain_technologies_known_by_others();
		}

		assert_throw(this->free_technology_count >= 0);

		if (this->free_technology_count > 0) {
			this->gain_free_technology();
		}

		const technology_set current_researches = this->get_current_researches();
		for (const technology *current_research : current_researches) {
			this->on_technology_researched(current_research);
		}

		this->current_researches.clear();
		emit current_researches_changed();
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
		this->decrease_population(true);
		++starvation_count;

		if (this->get_food_consumption() == 0) {
			this->set_population_growth(0);
			break;
		}
	}

	if (starvation_count > 0 && this->country == game::get()->get_player_country()) {
		const bool plural = starvation_count > 1;

		const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

		engine_interface::get()->add_notification("Starvation", interior_minister_portrait, std::format("Your Excellency, I regret to inform you that {} {} of our population {} starved to death.", number::to_formatted_string(starvation_count), (plural ? "units" : "unit"), (plural ? "have" : "has")));
	}
}

void country_game_data::do_everyday_consumption()
{
	if (this->get_population_units().empty()) {
		return;
	}

	const std::vector<population_unit *> population_units = vector::shuffled(this->get_population_units());

	for (population_unit *population_unit : population_units) {
		population_unit->set_everyday_consumption_fulfilled(true);
	}

	const int inflated_everyday_wealth_consumption = this->get_inflated_value(this->get_everyday_wealth_consumption());

	if (inflated_everyday_wealth_consumption > 0) {
		const int effective_consumption = std::max(0, std::min(inflated_everyday_wealth_consumption, this->get_wealth_with_credit()));

		if (effective_consumption > 0) {
			this->change_wealth(-effective_consumption);

			for (const auto &[population_type, count] : this->get_population()->get_type_counts()) {
				if (population_type->get_everyday_wealth_consumption() == 0) {
					continue;
				}

				const int population_type_consumption = this->get_inflated_value(population_type->get_everyday_wealth_consumption() * count);
				this->country->get_turn_data()->add_expense_transaction(expense_transaction_type::population_upkeep, population_type_consumption, population_type, count);
			}

			int remaining_consumption = inflated_everyday_wealth_consumption - effective_consumption;
			if (remaining_consumption != 0) {
				for (population_unit *population_unit : population_units) {
					const int pop_consumption = this->get_inflated_value(population_unit->get_type()->get_everyday_wealth_consumption());
					if (pop_consumption == 0) {
						continue;
					}

					population_unit->set_everyday_consumption_fulfilled(false);
					const int remaining_consumption_change = std::min(remaining_consumption, pop_consumption);
					remaining_consumption -= remaining_consumption_change;

					this->country->get_turn_data()->add_expense_transaction(expense_transaction_type::population_upkeep, -remaining_consumption_change, population_unit->get_type(), -1);

					if (remaining_consumption <= 0) {
						break;
					}
				}
			}
		}
	}

	for (const auto &[commodity, consumption] : this->get_everyday_consumption()) {
		if (!commodity->is_enabled()) {
			continue;
		}

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
			const centesimal_int pop_consumption = population_unit->get_type()->get_everyday_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_everyday_consumption_fulfilled(false);
			remaining_consumption -= pop_consumption;

			if (remaining_consumption <= 0) {
				break;
			}
		}
	}

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_everyday_consumption();

		for (const site *site : province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population() || !site->get_game_data()->is_built()) {
				continue;
			}

			site->get_game_data()->do_everyday_consumption();
		}
	}

	static const centesimal_int militancy_change_for_unfulfilled_consumption("0.1");
	static const centesimal_int militancy_change_for_fulfilled_consumption("-0.1");

	for (population_unit *population_unit : population_units) {
		if (population_unit->is_everyday_consumption_fulfilled()) {
			population_unit->change_militancy(militancy_change_for_fulfilled_consumption);
		} else {
			population_unit->change_militancy(militancy_change_for_unfulfilled_consumption);
		}
	}

	//FIXME: make population units which couldn't have their consumption fulfilled be unhappy/refuse to work for the turn (and possibly demote when demotion is implemented)
}

void country_game_data::do_luxury_consumption()
{
	if (this->get_population_units().empty()) {
		return;
	}

	const std::vector<population_unit *> population_units = vector::shuffled(this->get_population_units());

	for (population_unit *population_unit : population_units) {
		population_unit->set_luxury_consumption_fulfilled(true);
	}

	for (const auto &[commodity, consumption] : this->get_luxury_consumption()) {
		if (!commodity->is_enabled()) {
			continue;
		}

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
			const centesimal_int pop_consumption = population_unit->get_type()->get_luxury_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_luxury_consumption_fulfilled(false);
			remaining_consumption -= pop_consumption;

			if (remaining_consumption <= 0) {
				break;
			}
		}
	}

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_luxury_consumption();

		for (const site *site : province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population() || !site->get_game_data()->is_built()) {
				continue;
			}

			site->get_game_data()->do_luxury_consumption();
		}
	}

	static const centesimal_int consciousness_change_for_fulfilled_consumption("0.1");
	static const centesimal_int militancy_change_for_fulfilled_consumption("-0.2");

	for (population_unit *population_unit : population_units) {
		if (population_unit->is_luxury_consumption_fulfilled()) {
			population_unit->change_consciousness(consciousness_change_for_fulfilled_consumption);
			population_unit->change_militancy(militancy_change_for_fulfilled_consumption);
		}
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

void country_game_data::do_trade(country_map<commodity_map<int>> &country_luxury_demands)
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

			if (defines::get()->get_prestige_commodity()->is_enabled()) {
				//give trade priority by opinion-weighted prestige
				const int lhs_opinion_weighted_prestige = this->get_opinion_weighted_prestige_for(lhs);
				const int rhs_opinion_weighted_prestige = this->get_opinion_weighted_prestige_for(rhs);

				if (lhs_opinion_weighted_prestige != rhs_opinion_weighted_prestige) {
					return lhs_opinion_weighted_prestige > rhs_opinion_weighted_prestige;
				}
			} else {
				//give trade priority by opinion
				const int lhs_opinion = this->get_opinion_of(lhs);
				const int rhs_opinion = this->get_opinion_of(rhs);

				if (lhs_opinion != rhs_opinion) {
					return lhs_opinion > rhs_opinion;
				}
			}

			return lhs->get_identifier() < rhs->get_identifier();
		});

		commodity_map<int> offers = this->get_offers();
		for (auto &[commodity, offer] : offers) {
			const int price = game::get()->get_price(commodity);

			for (const metternich::country *other_country : countries) {
				country_game_data *other_country_game_data = other_country->get_game_data();

				const int bid = other_country_game_data->get_bid(commodity);
				if (bid != 0) {
					int sold_quantity = std::min(offer, bid);
					sold_quantity = std::min(sold_quantity, other_country_game_data->get_wealth_with_credit() / price);

					if (sold_quantity > 0) {
						this->do_sale(other_country, commodity, sold_quantity, true);

						offer -= sold_quantity;

						if (offer == 0) {
							break;
						}
					}
				}

				int &demand = country_luxury_demands[other_country][commodity];
				if (demand > 0) {
					const int sold_quantity = std::min(offer, demand);

					if (sold_quantity > 0) {
						this->do_sale(country, commodity, sold_quantity, false);

						offer -= sold_quantity;
						demand -= sold_quantity;

						if (offer == 0) {
							break;
						}
					}
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

		this->change_inflation(this->get_inflation_change());
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

	const bool is_last_turn_of_year = game::get()->get_year() != game::get()->get_next_date().year();
	if (is_last_turn_of_year) {
		country_event::check_events_for_scope(this->country, event_trigger::yearly_pulse);
	}

	const bool is_last_turn_of_quarter = is_last_turn_of_year || (game::get()->get_date().month() - 1) / 4 != (game::get()->get_next_date().month() - 1) / 4;
	if (is_last_turn_of_quarter) {
		country_event::check_events_for_scope(this->country, event_trigger::quarterly_pulse);
	}
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
		country_tier_data::get(this->get_tier())->get_modifier()->remove(this->country);
	}

	this->tier = tier;

	if (this->get_tier() != country_tier::none) {
		country_tier_data::get(this->get_tier())->get_modifier()->apply(this->country);
	}

	if (game::get()->is_running()) {
		emit tier_changed();
	}
}

const std::string &country_game_data::get_name() const
{
	return this->country->get_name(this->get_government_type(), this->get_tier());
}

std::string country_game_data::get_titled_name() const
{
	return this->country->get_titled_name(this->get_government_type(), this->get_tier(), this->get_religion());
}

const std::string &country_game_data::get_title_name() const
{
	return this->country->get_title_name(this->get_government_type(), this->get_tier(), this->get_religion());
}

const std::string &country_game_data::get_office_title_name(const office *office) const
{
	const character *office_holder = this->get_office_holder(office);
	const gender gender = office_holder != nullptr ? office_holder->get_gender() : gender::male;
	return this->country->get_office_title_name(office, this->get_government_type(), this->get_tier(), gender, this->get_religion());
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

	this->check_technologies();

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

		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, -count);
		}
	}

	this->overlord = overlord;

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(this->get_economic_score() * country_game_data::vassal_tax_rate / 100);

		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count);
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
		case country_type::great_power:
		case country_type::clade:
			if (this->get_overlord() != nullptr) {
				return "Subject Power";
			}

			if (this->get_rank() != nullptr) {
				return this->get_rank()->get_name();
			}

			return "Great Power";
		case country_type::minor_nation:
		case country_type::tribe:
			return "Minor Nation";
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

	this->check_government_type();
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

	for (const auto &[terrain, count] : province_game_data->get_tile_terrain_counts()) {
		this->change_tile_terrain_count(terrain, count * multiplier);
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

	if (game::get()->is_running()) {
		this->country->get_turn_data()->set_transport_level_recalculation_needed(true);
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

		for (const auto &[profession, capacity] : site_game_data->get_profession_capacities()) {
			this->change_profession_capacity(profession, capacity * multiplier);
		}
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

QVariantList country_game_data::get_resource_counts_qvariant_list() const
{
	return archimedes::map::to_value_sorted_qvariant_list(this->get_resource_counts());
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

	if (other_country->get_game_data()->is_clade()) {
		return true;
	} else if (this->is_clade()) {
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

	if (other_country->get_game_data()->is_tribal() || this->is_tribal()) {
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
	if (this->is_tribal() || this->is_clade()) {
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
	this->change_everyday_wealth_consumption(type->get_everyday_wealth_consumption() * change);

	for (const auto &[commodity, value] : type->get_everyday_consumption()) {
		if (!commodity->is_enabled()) {
			continue;
		}

		if (commodity->is_local()) {
			//handled at the site or province level
			continue;
		}

		this->change_everyday_consumption(commodity, value * change);
	}

	for (const auto &[commodity, value] : type->get_luxury_consumption()) {
		if (!commodity->is_enabled()) {
			continue;
		}

		if (commodity->is_local()) {
			//handled at the site or province level
			continue;
		}

		this->change_luxury_consumption(commodity, value * change);
	}

	//countries generate demand in the world market depending on population commodity demand
	for (const auto &[commodity, value] : type->get_commodity_demands()) {
		this->change_commodity_demand(commodity, value * change);
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

void country_game_data::change_profession_capacity(const profession *profession, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->profession_capacities[profession] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->profession_capacities.erase(profession);
	}
}

int country_game_data::get_available_profession_capacity(const profession *profession) const
{
	return this->get_profession_capacity(profession) - this->get_population()->get_profession_count(profession);
}

QVariantList country_game_data::get_population_type_inputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_type_inputs());
}

void country_game_data::change_population_type_input(const population_type *population_type, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_type_inputs[population_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_type_inputs.erase(population_type);
	}

	if (game::get()->is_running()) {
		emit population_type_inputs_changed();
	}
}

QVariantList country_game_data::get_population_type_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_type_outputs());
}

void country_game_data::change_population_type_output(const population_type *population_type, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_type_outputs[population_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_type_outputs.erase(population_type);
	}

	if (game::get()->is_running()) {
		emit population_type_outputs_changed();
	}
}

population_unit *country_game_data::choose_education_population_unit(const education_type *education_type)
{
	std::vector<population_unit *> population_units;

	for (population_unit *population_unit : this->get_population_units()) {
		if (population_unit->get_type() != education_type->get_input_population_type()) {
			continue;
		}

		if (!population_unit->get_site()->get_game_data()->can_have_population_type(education_type->get_output_population_type())) {
			continue;
		}

		if (education_type->get_output_population_type()->get_profession() != nullptr && population_unit->get_site()->get_game_data()->get_available_profession_capacity(education_type->get_output_population_type()->get_profession()) < 1) {
			continue;
		}

		population_units.push_back(population_unit);
	}

	if (population_units.empty()) {
		return nullptr;
	}

	return vector::get_random(population_units);
}

QVariantList country_game_data::get_building_slots_qvariant_list() const
{
	std::vector<country_building_slot *> available_building_slots;

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

void country_game_data::clear_buildings()
{
	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		building_slot->set_building(nullptr);
	}
}


bool country_game_data::check_free_building(const building_type *building)
{
	assert_throw(!building->is_provincial());

	if (building != this->country->get_culture()->get_building_class_type(building->get_building_class())) {
		return false;
	}

	if (this->has_building_or_better(building)) {
		return false;
	}

	country_building_slot *building_slot = this->get_building_slot(building->get_slot_type());

	if (building_slot == nullptr) {
		return false;
	}

	if (!building_slot->can_gain_building(building)) {
		return false;
	}

	if (building->get_required_building() != nullptr && building_slot->get_building() != building->get_required_building()) {
		return false;
	}

	if (building->get_required_technology() != nullptr && !this->has_technology(building->get_required_technology())) {
		return false;
	}

	building_slot->set_building(building);
	return true;
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

void country_game_data::add_taxable_wealth(const int taxable_wealth, const income_transaction_type tax_income_type)
{
	assert_throw(taxable_wealth >= 0);
	assert_throw(tax_income_type == income_transaction_type::tariff || tax_income_type == income_transaction_type::treasure_fleet);

	if (taxable_wealth == 0) {
		return;
	}

	if (this->get_overlord() == nullptr) {
		this->change_wealth(taxable_wealth);
		return;
	}

	const int tax = taxable_wealth * country_game_data::vassal_tax_rate / 100;
	const int taxed_wealth = taxable_wealth - tax;

	this->get_overlord()->get_game_data()->add_taxable_wealth(tax, tax_income_type);

	this->change_wealth(taxed_wealth);

	if (tax != 0) {
		this->get_overlord()->get_turn_data()->add_income_transaction(tax_income_type, tax, nullptr, 0, this->country);
		this->country->get_turn_data()->add_expense_transaction(expense_transaction_type::tax, tax, nullptr, 0, this->get_overlord());
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

		for (const education_type *education_type : building_slot->get_available_education_types()) {
			if (education_type->get_input_wealth() == 0) {
				continue;
			}

			const int input_wealth = building_slot->get_education_type_input_wealth(education_type);
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

		for (const education_type *education_type : building_slot->get_available_education_types()) {
			if (education_type->get_input_wealth() == 0) {
				continue;
			}

			const int input_wealth = building_slot->get_education_type_input_wealth(education_type);
			this->change_wealth(-input_wealth);
			this->change_wealth_income(-input_wealth);
		}
	}

	emit inflation_changed();
}

void country_game_data::set_inflation_change(const centesimal_int &inflation_change)
{
	if (inflation_change == this->get_inflation_change()) {
		return;
	}

	this->inflation_change = inflation_change;
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
	if (!commodity->is_enabled()) {
		return;
	}

	if (value == this->get_stored_commodity(commodity)) {
		return;
	}

	if (value < 0 && !commodity->is_negative_allowed()) {
		throw std::runtime_error("Tried to set the storage of commodity \"" + commodity->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\" to a negative number.");
	}

	if (commodity->is_convertible_to_wealth()) {
		assert_throw(value > 0);
		const int wealth_conversion_income = commodity->get_wealth_value() * value;
		this->add_taxable_wealth(wealth_conversion_income, income_transaction_type::treasure_fleet);
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
		this->change_economic_score(-change * commodity->get_base_price());
	}

	if (game::get()->is_running()) {
		emit commodity_inputs_changed();
	}
}

QVariantList country_game_data::get_transportable_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_transportable_commodity_outputs());
}

int country_game_data::get_transportable_commodity_output(const QString &commodity_identifier) const
{
	return this->get_transportable_commodity_output(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_game_data::change_transportable_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	if (commodity->is_abstract()) {
		this->change_commodity_output(commodity, change);
		return;
	}

	const centesimal_int new_output = (this->transportable_commodity_outputs[commodity] += change);

	assert_throw(new_output >= 0);

	if (new_output == 0) {
		this->transportable_commodity_outputs.erase(commodity);
	}

	const int transported_output = this->get_transported_commodity_output(commodity);
	if (new_output < transported_output) {
		this->change_transported_commodity_output(commodity, new_output.to_int() - transported_output);
	}

	if (game::get()->is_running()) {
		emit transportable_commodity_outputs_changed();
	}
}

QVariantList country_game_data::get_transported_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_transported_commodity_outputs());
}

void country_game_data::change_transported_commodity_output(const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int new_output = (this->transported_commodity_outputs[commodity] += change);

	assert_throw(new_output >= 0);
	assert_throw(new_output <= this->get_transportable_commodity_output(commodity).to_int());

	if (new_output == 0) {
		this->transported_commodity_outputs.erase(commodity);
	}

	this->change_commodity_output(commodity, centesimal_int(change));

	if (game::get()->is_running()) {
		emit transported_commodity_outputs_changed();
	}
}

QVariantList country_game_data::get_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_outputs());
}

int country_game_data::get_commodity_output(const QString &commodity_identifier) const
{
	return this->get_commodity_output(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_game_data::change_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int old_output = this->get_commodity_output(commodity);

	if (commodity->get_base_price() != 0 || commodity->get_wealth_value() != 0) {
		const int commodity_value = commodity->get_base_price() != 0 ? commodity->get_base_price() : commodity->get_wealth_value();
		this->change_economic_score(-old_output.to_int() * commodity_value);
	}

	const centesimal_int &new_output = (this->commodity_outputs[commodity] += change);

	assert_throw(new_output >= 0);

	if (new_output == 0) {
		this->commodity_outputs.erase(commodity);
	}

	if (commodity->get_base_price() != 0 || commodity->get_wealth_value() != 0) {
		const int commodity_value = commodity->get_base_price() != 0 ? commodity->get_base_price() : commodity->get_wealth_value();
		this->change_economic_score(new_output.to_int() * commodity_value);
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

void country_game_data::calculate_site_commodity_outputs()
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->calculate_site_commodity_outputs();
	}
}

void country_game_data::calculate_site_commodity_output(const commodity *commodity)
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->calculate_site_commodity_output(commodity);
	}
}

int country_game_data::get_food_output() const
{
	int food_output = 0;

	for (const auto &[commodity, output] : this->get_commodity_outputs()) {
		if (commodity->is_food()) {
			food_output += output.to_int();
		}
	}

	return food_output;
}

void country_game_data::change_everyday_wealth_consumption(const int change)
{
	if (change == 0) {
		return;
	}

	this->everyday_wealth_consumption += change;

	if (game::get()->is_running()) {
		emit everyday_wealth_consumption_changed();
	}
}

QVariantList country_game_data::get_everyday_consumption_qvariant_list() const
{
	commodity_map<int> int_everyday_consumption;

	for (const auto &[commodity, consumption] : this->get_everyday_consumption()) {
		int_everyday_consumption[commodity] = consumption.to_int();
	}

	return archimedes::map::to_qvariant_list(int_everyday_consumption);
}

int country_game_data::get_everyday_consumption(const QString &commodity_identifier) const
{
	return this->get_everyday_consumption(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_game_data::change_everyday_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->everyday_consumption[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->everyday_consumption.erase(commodity);
	}

	if (game::get()->is_running()) {
		emit everyday_consumption_changed();
	}
}

QVariantList country_game_data::get_luxury_consumption_qvariant_list() const
{
	commodity_map<int> int_luxury_consumption;

	for (const auto &[commodity, consumption] : this->get_luxury_consumption()) {
		int_luxury_consumption[commodity] = consumption.to_int();
	}

	return archimedes::map::to_qvariant_list(int_luxury_consumption);
}

int country_game_data::get_luxury_consumption(const QString &commodity_identifier) const
{
	return this->get_luxury_consumption(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_game_data::change_luxury_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->luxury_consumption[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->luxury_consumption.erase(commodity);
	}

	if (game::get()->is_running()) {
		emit luxury_consumption_changed();
	}
}

void country_game_data::change_commodity_demand(const commodity *commodity, const decimillesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const decimillesimal_int count = (this->commodity_demands[commodity] += change);

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

		for (const education_type *education_type : building_slot->get_available_education_types()) {
			if (education_type->get_input_wealth() == 0) {
				continue;
			}

			if (!building_slot->can_decrease_education(education_type)) {
				continue;
			}

			building_slot->decrease_education(education_type, restore_inputs);
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

		for (const education_type *education_type : building_slot->get_available_education_types()) {
			if (!education_type->get_input_commodities().contains(commodity)) {
				continue;
			}

			if (!building_slot->can_decrease_education(education_type)) {
				continue;
			}

			building_slot->decrease_education(education_type, restore_inputs);
			return;
		}
	}

	assert_throw(false);
}

bool country_game_data::produces_commodity(const commodity *commodity) const
{
	if (this->get_commodity_output(commodity).to_int() > 0) {
		return true;
	}

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->produces_commodity(commodity)) {
			return true;
		}
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() == commodity && building_slot->get_production_type_output(production_type).to_int() > 0) {
				return true;
			}
		}
	}

	return false;
}

void country_game_data::set_land_transport_capacity(const int capacity)
{
	if (capacity == this->get_land_transport_capacity()) {
		return;
	}

	this->land_transport_capacity = capacity;

	if (game::get()->is_running()) {
		emit land_transport_capacity_changed();
	}
}

void country_game_data::set_sea_transport_capacity(const int capacity)
{
	if (capacity == this->get_sea_transport_capacity()) {
		return;
	}

	this->sea_transport_capacity = capacity;

	if (game::get()->is_running()) {
		emit sea_transport_capacity_changed();
	}
}

void country_game_data::assign_transport_orders()
{
	if (this->is_under_anarchy()) {
		return;
	}

	for (const auto &[commodity, transportable_output] : this->get_transportable_commodity_outputs()) {
		const int available_transportable_output = transportable_output.to_int() - this->get_transported_commodity_output(commodity);
		assert_throw(available_transportable_output >= 0);
		if (available_transportable_output == 0) {
			continue;
		}

		const int available_transport_capacity = this->get_available_transport_capacity();
		if (available_transport_capacity == 0) {
			break;
		}

		this->change_transported_commodity_output(commodity, std::min(available_transportable_output, available_transport_capacity));
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

QVariantList country_game_data::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

void country_game_data::add_technology(const technology *technology)
{
	if (this->has_technology(technology)) {
		return;
	}

	assert_throw(technology->is_available_for_country(this->country));

	this->technologies.insert(technology);

	if (technology->get_modifier() != nullptr) {
		technology->get_modifier()->apply(this->country, 1);
	}

	for (const commodity *enabled_commodity : technology->get_enabled_commodities()) {
		if (!enabled_commodity->is_enabled()) {
			continue;
		}

		this->add_available_commodity(enabled_commodity);

		if (enabled_commodity->is_tradeable()) {
			this->add_tradeable_commodity(enabled_commodity);
		}
	}

	for (const resource *discovered_resource : technology->get_enabled_resources()) {
		if (discovered_resource->is_prospectable()) {
			const point_set prospected_tiles = this->prospected_tiles;
			for (const QPoint &tile_pos : prospected_tiles) {
				const tile *tile = map::get()->get_tile(tile_pos);

				if (tile->is_resource_discovered()) {
					continue;
				}

				if (!vector::contains(discovered_resource->get_terrain_types(), tile->get_terrain())) {
					continue;
				}

				this->reset_tile_prospection(tile_pos);
			}
		} else {
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
	}

	//check if any discoveries can now be triggered, if they required this technology
	bool leads_to_discovery = false;
	for (const metternich::technology *requiring_technology : technology->get_leads_to()) {
		if (requiring_technology->is_discovery()) {
			leads_to_discovery = true;
			break;
		}
	}

	if (leads_to_discovery) {
		for (const province *province : this->explored_provinces) {
			const province_game_data *province_game_data = province->get_game_data();

			for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
				const tile *tile = map::get()->get_tile(tile_pos);
				const resource *tile_resource = tile->get_resource();

				if (!tile->is_resource_discovered()) {
					continue;
				}

				if (tile_resource->get_discovery_technology() == nullptr) {
					continue;
				}

				if (this->can_gain_technology(tile_resource->get_discovery_technology())) {
					this->add_technology(tile_resource->get_discovery_technology());

					if (game::get()->is_running()) {
						emit technology_researched(tile_resource->get_discovery_technology());
					}
				}
			}
		}

		for (const QPoint &tile_pos : this->explored_tiles) {
			const tile *tile = map::get()->get_tile(tile_pos);
			const resource *tile_resource = tile->get_resource();

			if (tile_resource == nullptr) {
				continue;
			}

			if (!tile->is_resource_discovered()) {
				continue;
			}

			if (tile_resource->get_discovery_technology() == nullptr) {
				continue;
			}

			if (this->can_gain_technology(tile_resource->get_discovery_technology())) {
				this->add_technology(tile_resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit technology_researched(tile_resource->get_discovery_technology());
				}
			}
		}
	}

	if (game::get()->is_running()) {
		if (!technology->get_enabled_laws().empty()) {
			this->check_laws();
		}

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

void country_game_data::remove_technology(const technology *technology)
{
	assert_throw(technology != nullptr);

	if (!this->has_technology(technology)) {
		return;
	}

	this->technologies.erase(technology);

	if (technology->get_modifier() != nullptr) {
		technology->get_modifier()->apply(this->country, -1);
	}

	for (const commodity *enabled_commodity : technology->get_enabled_commodities()) {
		if (!enabled_commodity->is_enabled()) {
			continue;
		}

		this->remove_available_commodity(enabled_commodity);

		if (enabled_commodity->is_tradeable()) {
			this->remove_tradeable_commodity(enabled_commodity);
		}
	}

	if (!technology->get_enabled_laws().empty()) {
		this->check_laws();
	}

	//remove any technologies requiring this one as well
	for (const metternich::technology *requiring_technology : technology->get_leads_to()) {
		this->remove_technology(requiring_technology);
	}

	if (game::get()->is_running()) {
		emit technology_lost(technology);

		emit technologies_changed();
	}
}

void country_game_data::check_technologies()
{
	//technologies may no longer be available for the country due to e.g. religion change, and may therefore need to be removed

	const technology_set technologies = this->get_technologies();
	for (const technology *technology : technologies) {
		if (!technology->is_available_for_country(this->country)) {
			this->remove_technology(technology);
		}
	}
}

bool country_game_data::can_gain_technology(const technology *technology) const
{
	assert_throw(technology != nullptr);

	if (!technology->is_available_for_country(this->country)) {
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

bool country_game_data::can_research_technology(const technology *technology) const
{
	if (!this->can_gain_technology(technology)) {
		return false;
	}

	const int wealth_cost = technology->get_wealth_cost_for_country(this->country);
	if (wealth_cost > 0 && this->get_inflated_value(wealth_cost) > this->get_wealth_with_credit()) {
		return false;
	}

	for (const auto &[commodity, cost] : technology->get_commodity_costs_for_country(this->country)) {
		if (cost > this->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return true;
}

std::vector<const technology *> country_game_data::get_researchable_technologies() const
{
	std::vector<const technology *> researchable_technologies;

	for (const technology *technology : this->country->get_available_technologies()) {
		if (!this->is_technology_researchable(technology)) {
			continue;
		}

		researchable_technologies.push_back(technology);
	}

	std::sort(researchable_technologies.begin(), researchable_technologies.end(), technology_compare());

	return researchable_technologies;
}

QVariantList country_game_data::get_researchable_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_researchable_technologies());
}

bool country_game_data::is_technology_researchable(const technology *technology) const
{
	if (technology->is_discovery()) {
		return false;
	}

	return this->can_gain_technology(technology);
}

QVariantList country_game_data::get_future_technologies_qvariant_list() const
{
	std::vector<const technology *> future_technologies = this->country->get_available_technologies();
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

QVariantList country_game_data::get_current_researches_qvariant_list() const
{
	return container::to_qvariant_list(this->get_current_researches());
}

void country_game_data::add_current_research(const technology *technology)
{
	assert_throw(this->can_research_technology(technology));

	const int wealth_cost = technology->get_wealth_cost_for_country(this->country);
	this->change_wealth_inflated(-wealth_cost);

	for (const auto &[commodity, cost] : technology->get_commodity_costs_for_country(this->country)) {
		this->change_stored_commodity(commodity, -cost);
	}

	this->current_researches.insert(technology);
	emit current_researches_changed();
}

void country_game_data::remove_current_research(const technology *technology, const bool restore_costs)
{
	assert_throw(this->get_current_researches().contains(technology));

	if (restore_costs) {
		const int wealth_cost = technology->get_wealth_cost_for_country(this->country);
		this->change_wealth_inflated(wealth_cost);

		for (const auto &[commodity, cost] : technology->get_commodity_costs_for_country(this->country)) {
			this->change_stored_commodity(commodity, cost);
		}
	}

	this->current_researches.erase(technology);
	emit current_researches_changed();
}

void country_game_data::on_technology_researched(const technology *technology)
{
	if (this->get_current_researches().contains(technology)) {
		this->remove_current_research(technology, false);
	}

	this->add_technology(technology);

	if (technology->get_free_technologies() > 0) {
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
			this->gain_free_technologies(technology->get_free_technologies());
		}
	}

	if (technology->get_shared_prestige() > 0 && defines::get()->get_prestige_commodity()->is_enabled()) {
		this->change_stored_commodity(defines::get()->get_prestige_commodity(), technology->get_shared_prestige_for_country(this->country));
	}

	emit technology_researched(technology);
}

data_entry_map<technology_category, const technology *> country_game_data::get_research_choice_map(const bool is_free) const
{
	const std::vector<const technology *> researchable_technologies = this->get_researchable_technologies();

	if (researchable_technologies.empty()) {
		return {};
	}

	data_entry_map<technology_category, std::vector<const technology *>> potential_technologies_per_category;

	for (const technology *technology : researchable_technologies) {
		if (!is_free && !this->can_research_technology(technology)) {
			continue;
		}

		std::vector<const metternich::technology *> &category_technologies = potential_technologies_per_category[technology->get_category()];

		const int weight = 1;
		for (int i = 0; i < weight; ++i) {
			category_technologies.push_back(technology);
		}
	}

	if (is_free) {
		assert_throw(!potential_technologies_per_category.empty());
	}

	if (potential_technologies_per_category.empty()) {
		return {};
	}

	data_entry_map<technology_category, const technology *> research_choice_map;
	const std::vector<const technology_category *> potential_categories = archimedes::map::get_keys(potential_technologies_per_category);

	for (const technology_category *category : potential_categories) {
		research_choice_map[category] = vector::get_random(potential_technologies_per_category[category]);
	}

	return research_choice_map;
}

void country_game_data::gain_free_technology()
{
	const data_entry_map<technology_category, const technology *> research_choice_map = this->get_research_choice_map(true);

	if (research_choice_map.empty()) {
		return;
	}

	if (this->is_ai()) {
		const technology *chosen_technology = this->get_ai()->get_research_choice(research_choice_map);
		this->gain_free_technology(chosen_technology);
	} else {
		const std::vector<const technology *> potential_technologies = archimedes::map::get_values(research_choice_map);
		emit engine_interface::get()->free_technology_choosable(container::to_qvariant_list(potential_technologies));
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

	if (government_type != nullptr) {
		if (this->country->is_tribe() && !government_type->get_group()->is_tribal()) {
			throw std::runtime_error(std::format("Tried to set a non-tribal government type (\"{}\") for a tribal country (\"{}\").", government_type->get_identifier(), this->country->get_identifier()));
		}

		if (this->country->is_clade() && !government_type->get_group()->is_clade()) {
			throw std::runtime_error(std::format("Tried to set a non-clade government type (\"{}\") for a clade country (\"{}\").", government_type->get_identifier(), this->country->get_identifier()));
		}
	}

	if (this->get_government_type() != nullptr && this->get_government_type()->get_modifier() != nullptr) {
		this->get_government_type()->get_modifier()->apply(this->country, -1);
	}

	this->government_type = government_type;

	if (this->get_government_type() != nullptr && this->get_government_type()->get_modifier() != nullptr) {
		this->get_government_type()->get_modifier()->apply(this->country, 1);
	}

	if (game::get()->is_running()) {
		emit government_type_changed();
	}
}

bool country_game_data::can_have_government_type(const metternich::government_type *government_type) const
{
	if (government_type->get_required_technology() != nullptr && !this->has_technology(government_type->get_required_technology())) {
		return false;
	}

	for (const law *forbidden_law : government_type->get_forbidden_laws()) {
		if (this->has_law(forbidden_law)) {
			return false;
		}
	}

	if (government_type->get_conditions() != nullptr && !government_type->get_conditions()->check(this->country, read_only_context(this->country))) {
		return false;
	}

	return true;
}

void country_game_data::check_government_type()
{
	if (this->get_government_type() != nullptr && this->can_have_government_type(this->get_government_type())) {
		return;
	}

	std::vector<const metternich::government_type *> potential_government_types;

	for (const metternich::government_type *government_type : government_type::get_all()) {
		if (this->can_have_government_type(government_type)) {
			potential_government_types.push_back(government_type);
		}
	}

	assert_throw(!potential_government_types.empty());

	this->set_government_type(vector::get_random(potential_government_types));
}

bool country_game_data::is_tribal() const
{
	return this->get_government_type()->get_group()->is_tribal();
}

bool country_game_data::is_clade() const
{
	return this->get_government_type()->get_group()->is_clade();
}

QVariantList country_game_data::get_laws_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_laws());
}

void country_game_data::set_law(const law_group *law_group, const law *law)
{
	assert_throw(law_group != nullptr);

	if (law == this->get_law(law_group)) {
		return;
	}

	const metternich::law *old_law = this->get_law(law_group);
	if (old_law != nullptr) {
		old_law->get_modifier()->remove(this->country);
	}

	this->laws[law_group] = law;

	if (law != nullptr) {
		assert_throw(law->get_group() == law_group);
		law->get_modifier()->apply(this->country);
	}

	this->check_government_type();

	if (game::get()->is_running()) {
		emit laws_changed();
	}
}

bool country_game_data::has_law(const law *law) const
{
	return this->get_law(law->get_group()) == law;
}

bool country_game_data::can_have_law(const metternich::law *law) const
{
	if (law->get_required_technology() != nullptr && !this->has_technology(law->get_required_technology())) {
		return false;
	}

	if (law->get_conditions() != nullptr && !law->get_conditions()->check(this->country, read_only_context(this->country))) {
		return false;
	}

	return true;
}

bool country_game_data::can_enact_law(const metternich::law *law) const
{
	if (!this->can_have_law(law)) {
		return false;
	}

	if (vector::contains(this->get_government_type()->get_forbidden_laws(), law)) {
		return false;
	}

	for (const auto &[commodity, cost] : law->get_commodity_costs()) {
		if (this->get_stored_commodity(commodity) < (cost * this->get_total_law_cost_modifier() / 100)) {
			return false;
		}
	}

	return true;
}

void country_game_data::enact_law(const law *law)
{
	for (const auto &[commodity, cost] : law->get_commodity_costs()) {
		this->change_stored_commodity(commodity, -cost * this->get_total_law_cost_modifier() / 100);
	}

	this->set_law(law->get_group(), law);
}

void country_game_data::check_laws()
{
	for (const law_group *law_group : law_group::get_all()) {
		if (this->get_law(law_group) != nullptr && !this->can_have_law(this->get_law(law_group))) {
			this->set_law(law_group, nullptr);
		}

		if (this->get_law(law_group) == nullptr) {
			const law *government_type_default_law = this->get_government_type()->get_default_law(law_group);
			if (government_type_default_law != nullptr && this->can_have_law(government_type_default_law)) {
				this->set_law(law_group, government_type_default_law);
			}
		}

		if (this->get_law(law_group) == nullptr) {
			if (this->can_have_law(law_group->get_default_law())) {
				this->set_law(law_group, law_group->get_default_law());
			}
		}

		if (this->get_law(law_group) == nullptr) {
			std::vector<const law *> potential_laws;
			for (const metternich::law *group_law : law_group->get_laws()) {
				if (this->can_have_law(group_law)) {
					potential_laws.push_back(group_law);
				}
			}
			if (!potential_laws.empty()) {
				this->set_law(law_group, vector::get_random(potential_laws));
			}
		}
	}
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
			const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

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
			this->change_stored_commodity(commodity, -cost);
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
			this->change_stored_commodity(commodity, cost);
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
	if (old_idea != nullptr && old_idea->get_obsolescence_technology() != nullptr && this->has_technology(old_idea->get_obsolescence_technology())) {
		this->set_idea(slot, nullptr);

		if (game::get()->is_running()) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

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
		if (this->get_stored_commodity(commodity) < cost) {
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

QVariantList country_game_data::get_available_research_organization_slots_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_idea_slots(idea_type::research_organization));
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

void country_game_data::check_characters()
{
	const std::vector<const office *> available_offices = this->get_available_offices();
	for (const office *office : available_offices) {
		this->check_office_holder(office, nullptr);
	}

	const data_entry_map<office, const character *> office_holders = this->get_office_holders();
	for (const auto &[office, office_holder] : office_holders) {
		if (!vector::contains(available_offices, office)) {
			this->set_office_holder(office, nullptr);
		}
	}

	this->appointed_office_holders.clear();

	this->check_advisors();

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->check_governor();

		for (const site *site : province->get_map_data()->get_sites()) {
			if (site->get_map_data()->get_type() == site_type::resource || site->get_map_data()->get_type() == site_type::celestial_body) {
				site->get_game_data()->check_landholder();
			}
		}
	}

	this->check_leaders();
	this->check_civilian_characters();
}

const character *country_game_data::get_ruler() const
{
	return this->get_office_holder(defines::get()->get_ruler_office());
}

QVariantList country_game_data::get_office_holders_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_office_holders());
}

void country_game_data::set_office_holder(const office *office, const character *character)
{
	const metternich::character *old_office_holder = this->get_office_holder(office);

	if (character == old_office_holder) {
		return;
	}

	if (old_office_holder != nullptr) {
		old_office_holder->get_game_data()->apply_office_modifier(this->country, office, -1);
		old_office_holder->get_game_data()->set_office(nullptr);
		old_office_holder->get_game_data()->set_country(nullptr);
	}

	const metternich::office *old_office = character ? character->get_game_data()->get_office() : nullptr;
	if (old_office != nullptr) {
		this->set_office_holder(old_office, nullptr);
	}

	if (character != nullptr) {
		this->office_holders[office] = character;
	} else {
		this->office_holders.erase(office);
	}

	if (character != nullptr) {
		character->get_game_data()->apply_office_modifier(this->country, office, 1);
		character->get_game_data()->set_office(office);
		character->get_game_data()->set_country(this->country);
	}

	if (old_office != nullptr) {
		this->check_office_holder(old_office, character);
	}

	if (office->is_ruler()) {
		if (this->country == game::get()->get_player_country()) {
			game::get()->set_player_character(character);
		}
	}

	if (game::get()->is_running()) {
		emit office_holders_changed();

		if (office->is_ruler()) {
			emit ruler_changed();

			if (old_office_holder != nullptr) {
				emit old_office_holder->get_game_data()->ruler_changed();
			}

			if (character != nullptr) {
				emit character->get_game_data()->ruler_changed();
			}
		}

		if (this->country == game::get()->get_player_country() && character != nullptr) {
			const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

			engine_interface::get()->add_notification(std::format("New {}", office->get_name()), interior_minister_portrait, std::format("{} has become our new {}!\n\n{}", character->get_full_name(), string::lowered(office->get_name()), character->get_game_data()->get_office_modifier_string(this->country, office)));
		}
	}
}

QVariantList country_game_data::get_appointed_office_holders_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_appointed_office_holders());
}

void country_game_data::set_appointed_office_holder(const office *office, const character *character)
{
	assert_throw(office->is_appointable());

	const metternich::character *old_appointee = this->get_appointed_office_holder(office);

	if (character == old_appointee) {
		return;
	}

	if (character != nullptr) {
		const commodity_map<int> commodity_costs = this->get_advisor_commodity_costs(office);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->change_stored_commodity(commodity, -cost);
		}

		this->appointed_office_holders[office] = character;
	} else {
		this->appointed_office_holders.erase(office);
	}

	if (old_appointee != nullptr) {
		const commodity_map<int> commodity_costs = this->get_advisor_commodity_costs(office);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->change_stored_commodity(commodity, cost);
		}
	}

	if (game::get()->is_running()) {
		emit appointed_office_holders_changed();
	}
}

void country_game_data::check_office_holder(const office *office, const character *previous_holder)
{
	if (this->is_under_anarchy()) {
		this->set_office_holder(office, nullptr);
		return;
	}

	//process appointment, if any
	const character *appointed_holder = this->get_appointed_office_holder(office);
	if (appointed_holder != nullptr && this->can_have_office_holder(office, appointed_holder)) {
		assert_throw(office->is_appointable());
		this->set_office_holder(office, appointed_holder);
		this->set_appointed_office_holder(office, nullptr);
	}

	//remove office holders if they have become obsolete
	if (this->get_office_holder(office) != nullptr && this->get_office_holder(office)->get_obsolescence_technology() != nullptr && this->has_technology(this->get_office_holder(office)->get_obsolescence_technology())) {
		this->get_office_holder(office)->get_game_data()->die();
		return; //the office holder death will already trigger a re-check of the office holder position
	}

	//if the country has no holder for a non-appointable office, see if there is any character who can become the holder
	if (this->get_office_holder(office) == nullptr && !office->is_appointable()) {
		const character *character = this->get_best_office_holder(office, previous_holder);
		if (character != nullptr) {
			this->set_office_holder(office, character);
		}
	}
}

std::vector<const character *> country_game_data::get_appointable_office_holders(const office *office) const
{
	assert_throw(this->get_government_type() != nullptr);

	std::vector<const character *> potential_holders;

	for (const character *character : character::get_all()) {
		if (office->is_ruler()) {
			if (character->get_role() != character_role::ruler) {
				continue;
			}
		} else {
			if (character->get_role() != character_role::advisor) {
				continue;
			}
		}

		if (!this->can_gain_office_holder(office, character)) {
			continue;
		}

		potential_holders.push_back(character);
	}

	return potential_holders;
}

QVariantList country_game_data::get_appointable_office_holders_qvariant_list(const office *office) const
{
	return container::to_qvariant_list(this->get_appointable_office_holders(office));
}

const character *country_game_data::get_best_office_holder(const office *office, const character *previous_holder) const
{
	assert_throw(this->get_government_type() != nullptr);

	std::vector<const character *> potential_holders;
	int best_attribute_value = 0;
	bool found_same_dynasty = false;

	for (const character *character : this->get_appointable_office_holders(office)) {
		if (!this->can_appoint_office_holder(office, character)) {
			continue;
		}

		const character_game_data *character_game_data = character->get_game_data();

		if (office->is_ruler()) {
			if (previous_holder != nullptr && previous_holder->get_dynasty() != nullptr) {
				const bool same_dynasty = character->get_dynasty() == previous_holder->get_dynasty();
				if (same_dynasty && !found_same_dynasty) {
					potential_holders.clear();
					best_attribute_value = 0;
					found_same_dynasty = true;
				} else if (!same_dynasty && found_same_dynasty) {
					continue;
				}
			}
		}

		const int attribute_value = character_game_data->get_attribute_value(office->get_attribute());

		if (attribute_value < best_attribute_value) {
			continue;
		}

		if (attribute_value > best_attribute_value) {
			best_attribute_value = attribute_value;
			potential_holders.clear();
		}

		potential_holders.push_back(character);
	}

	if (!potential_holders.empty()) {
		const character *office_holder = vector::get_random(potential_holders);
		return office_holder;
	}

	return nullptr;
}

bool country_game_data::can_have_office_holder(const office *office, const character *character) const
{
	const character_game_data *character_game_data = character->get_game_data();
	if (character_game_data->get_country() != nullptr && character_game_data->get_country() != this->country) {
		return false;
	}

	if (character_game_data->is_dead()) {
		return false;
	}

	if (character->get_required_technology() != nullptr && !this->has_technology(character->get_required_technology())) {
		return false;
	}

	if (character->get_obsolescence_technology() != nullptr && this->has_technology(character->get_obsolescence_technology())) {
		return false;
	}

	if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->country, read_only_context(this->country))) {
		return false;
	}

	if (office->is_ruler()) {
		if (!vector::contains(this->country->get_rulers(), character)) {
			return false;
		}
	} else {
		if (character_game_data->get_office() != nullptr) {
			return false;
		}
	}

	if (office->get_holder_conditions() != nullptr && !office->get_holder_conditions()->check(character, read_only_context(character))) {
		return false;
	}

	return true;
}

bool country_game_data::can_gain_office_holder(const office *office, const character *character) const
{
	if (!this->can_have_office_holder(office, character)) {
		return false;
	}

	for (const auto &[loop_office, office_appointee] : this->get_appointed_office_holders()) {
		if (office_appointee == character) {
			return false;
		}
	}

	return true;
}

bool country_game_data::can_appoint_office_holder(const office *office, const character *character) const
{
	if (!this->can_gain_office_holder(office, character)) {
		return false;
	}

	const commodity_map<int> commodity_costs = this->get_advisor_commodity_costs(office);
	for (const auto &[commodity, cost] : commodity_costs) {
		if (this->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	return true;
}

void country_game_data::on_office_holder_died(const office *office, const character *office_holder)
{
	if (game::get()->is_running()) {
		if (this->country == game::get()->get_player_country()) {
			const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

			if (office->is_ruler()) {
				engine_interface::get()->add_notification(std::format("{} Died", office_holder->get_full_name()), interior_minister_portrait, std::format("Our {}, {}, has died!", string::lowered(office->get_name()), office_holder->get_full_name()));
			} else {
				engine_interface::get()->add_notification(std::format("{} Retired", office_holder->get_full_name()), interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, {} {} has decided to retire.", string::lowered(office->get_name()), office_holder->get_full_name()));
			}
		}

		if (office->is_ruler()) {
			context ctx(this->country);
			ctx.source_scope = office_holder;
			country_event::check_events_for_scope(this->country, event_trigger::ruler_death, ctx);
		}
	}

	this->check_office_holder(office, office_holder);
}

std::vector<const office *> country_game_data::get_available_offices() const
{
	std::vector<const office *> available_offices;

	for (const office *office : office::get_all()) {
		if (office->get_conditions() != nullptr && !office->get_conditions()->check(this->country, read_only_context(this->country))) {
			continue;
		}

		available_offices.push_back(office);
	}

	return available_offices;
}

std::vector<const office *> country_game_data::get_appointable_available_offices() const
{
	std::vector<const office *> available_offices = this->get_available_offices();
	std::erase_if(available_offices, [](const office *office) {
		return !office->is_appointable();
	});
	return available_offices;
}

QVariantList country_game_data::get_available_offices_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_offices());
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
				const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Retired", interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the advisor {} has decided to retire.", advisor->get_full_name()));
			}

			this->remove_advisor(advisor);
			advisor->get_game_data()->set_dead(true);
		}
	}

	if (this->is_under_anarchy() || !this->can_have_advisors()) {
		if (this->get_next_advisor() != nullptr) {
			this->set_next_advisor(nullptr);
		}

		return;
	}

	if (this->get_next_advisor() != nullptr) {
		if (this->get_next_advisor()->get_game_data()->get_country() != nullptr) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Unavailable", interior_minister_portrait, std::format("Your Excellency, the advisor {} has unfortunately decided to join {}, and is no longer available for recruitment.", this->get_next_advisor()->get_full_name(), this->get_next_advisor()->get_game_data()->get_country()->get_game_data()->get_name()));
			}

			this->set_next_advisor(nullptr);
		} else if (this->get_next_advisor()->get_obsolescence_technology() != nullptr && this->has_technology(this->get_next_advisor()->get_obsolescence_technology())) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Unavailable", interior_minister_portrait, std::format("Your Excellency, the advisor {} is no longer available for recruitment.", this->get_next_advisor()->get_full_name()));
			}

			this->set_next_advisor(nullptr);
		} else {
			if (this->get_stored_commodity(defines::get()->get_advisor_commodity()) >= this->get_advisor_cost()) {
				this->change_stored_commodity(defines::get()->get_advisor_commodity(), -this->get_advisor_cost());

				this->add_advisor(this->get_next_advisor());

				for (const character_trait *trait : this->get_next_advisor()->get_game_data()->get_traits()) {
					if (trait->get_advisor_effects() != nullptr) {
						context ctx(this->country);
						trait->get_advisor_effects()->do_effects(this->country, ctx);
					}
				}

				emit advisor_recruited(this->get_next_advisor());

				this->set_next_advisor(nullptr);
			}
		}
	} else {
		if (this->get_commodity_output(defines::get()->get_advisor_commodity()).to_int() > 0 || this->get_stored_commodity(defines::get()->get_advisor_commodity()) > 0) {
			this->choose_next_advisor();
		}
	}
}

void country_game_data::add_advisor(const character *advisor)
{
	try {
		assert_throw(this->can_have_advisors());
		assert_throw(advisor->get_role() == character_role::advisor);

		const character *replaced_advisor = this->get_replaced_advisor_for(advisor);
		if (replaced_advisor != nullptr) {
			this->remove_advisor(replaced_advisor);
		}

		this->advisors.push_back(advisor);
		advisor->get_game_data()->set_country(this->country);
		advisor->get_game_data()->apply_advisor_modifier(this->country, 1);

		emit advisors_changed();
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to add advisor \"{}\" for country \"{}\".", advisor->get_identifier(), this->country->get_identifier())));
	}
}

void country_game_data::remove_advisor(const character *advisor)
{
	assert_throw(advisor->get_game_data()->get_country() == this->country);

	std::erase(this->advisors, advisor);
	advisor->get_game_data()->set_country(nullptr);
	advisor->get_game_data()->apply_advisor_modifier(this->country, -1);

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

commodity_map<int> country_game_data::get_advisor_commodity_costs(const office *office) const
{
	commodity_map<int> commodity_costs;

	if (office != nullptr && office->is_appointable()) {
		const int cost = this->get_advisor_cost();
		commodity_costs[defines::get()->get_advisor_commodity()] = cost;
	}

	return commodity_costs;
}

QVariantList country_game_data::get_advisor_commodity_costs_qvariant_list(const office *office) const
{
	return archimedes::map::to_qvariant_list(this->get_advisor_commodity_costs(office));
}

void country_game_data::choose_next_advisor()
{
	std::map<advisor_category, std::vector<const character *>> potential_advisors_per_category;

	for (const character *character : character::get_all()) {
		if (character->get_role() != character_role::advisor) {
			continue;
		}

		if (!this->can_recruit_advisor(character)) {
			continue;
		}

		int weight = 1;

		if (character->get_home_settlement()->get_game_data()->get_owner() == this->country) {
			weight += 4;
		}

		std::vector<const metternich::character *> &category_advisors = potential_advisors_per_category[character->get_character_type()->get_advisor_category()];

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
			int advisor_skill = advisor->get_game_data()->get_primary_attribute_value();
			for (const character_trait *trait : advisor->get_game_data()->get_traits_of_type(character_trait_type::advisor)) {
				if ((trait->get_advisor_modifier() != nullptr || trait->get_advisor_effects() != nullptr) && trait->get_scaled_advisor_modifier() == nullptr) {
					advisor_skill = defines::get()->get_max_character_skill();
					break;
				}
			}

			int desire = advisor_skill * 100;

			for (const journal_entry *journal_entry : this->get_active_journal_entries()) {
				if (vector::contains(journal_entry->get_recruited_characters(), advisor)) {
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
	return this->country->is_great_power() && defines::get()->get_advisors_game_rule() != nullptr && game::get()->get_rules()->get_value(defines::get()->get_advisors_game_rule());
}

bool country_game_data::can_recruit_advisor(const character *advisor) const
{
	const character_game_data *advisor_game_data = advisor->get_game_data();
	if (advisor_game_data->get_country() != nullptr) {
		return false;
	}

	if (advisor_game_data->is_dead()) {
		return false;
	}

	if (advisor->get_required_technology() != nullptr && !this->has_technology(advisor->get_required_technology())) {
		return false;
	}

	if (advisor->get_obsolescence_technology() != nullptr && this->has_technology(advisor->get_obsolescence_technology())) {
		return false;
	}

	if (advisor->get_conditions() != nullptr && !advisor->get_conditions()->check(this->country, read_only_context(this->country))) {
		return false;
	}

	if (this->has_incompatible_advisor_to(advisor)) {
		return false;
	}

	return true;
}

bool country_game_data::has_incompatible_advisor_to(const character *advisor) const
{
	const std::vector<const character_trait *> advisor_traits = advisor->get_game_data()->get_traits_of_type(character_trait_type::advisor);

	//only one advisor with the same advisor type can be obtained (though advisors can be replaced with higher-skilled ones)
	for (const character *other_advisor : this->get_advisors()) {
		if (other_advisor->get_character_type() != advisor->get_character_type()) {
			continue;
		}

		if (other_advisor->get_game_data()->get_traits_of_type(character_trait_type::advisor) != advisor_traits) {
			continue;
		}

		bool has_better_attribute = false;
		for (const character_trait *advisor_trait : advisor_traits) {
			if (advisor_trait->get_scaled_advisor_modifier() != nullptr && advisor->get_game_data()->get_attribute_value(advisor_trait->get_attribute()) > other_advisor->get_game_data()->get_attribute_value(advisor_trait->get_attribute())) {
				has_better_attribute = true;
				break;
			}
		}
		if (has_better_attribute) {
			continue;
		}

		return true;
	}

	return false;
}

const character *country_game_data::get_replaced_advisor_for(const character *advisor) const
{
	const std::vector<const character_trait *> advisor_traits = advisor->get_game_data()->get_traits_of_type(character_trait_type::advisor);

	//only one advisor with the same advisor trait can be obtained (though advisors can be replaced with higher-skilled ones)
	for (const character *other_advisor : this->get_advisors()) {
		if (other_advisor->get_character_type() != advisor->get_character_type()) {
			continue;
		}

		if (other_advisor->get_game_data()->get_traits_of_type(character_trait_type::advisor) != advisor_traits) {
			continue;
		}

		return other_advisor;
	}

	return nullptr;
}

bool country_game_data::can_have_advisors_or_appointable_offices() const
{
	return this->can_have_advisors() || !this->get_appointable_available_offices().empty();
}

const metternich::portrait *country_game_data::get_interior_minister_portrait() const
{
	const character *office_holder = this->get_office_holder(defines::get()->get_interior_minister_office());
	if (office_holder != nullptr) {
		return office_holder->get_game_data()->get_portrait();
	}

	return defines::get()->get_interior_minister_portrait();
}

const metternich::portrait *country_game_data::get_war_minister_portrait() const
{
	const character *office_holder = this->get_office_holder(defines::get()->get_war_minister_office());
	if (office_holder != nullptr) {
		return office_holder->get_game_data()->get_portrait();
	}

	return defines::get()->get_war_minister_portrait();
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
				const portrait *war_minister_portrait = this->get_war_minister_portrait();

				const std::string_view leader_type_name = leader->get_leader_type_name();

				engine_interface::get()->add_notification(std::format("{} Retired", leader_type_name), war_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the {} {} has decided to retire.", string::lowered(leader_type_name), leader->get_full_name()));
			}

			leader->get_game_data()->get_military_unit()->disband(true);
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
				const portrait *war_minister_portrait = this->get_war_minister_portrait();

				const std::string_view leader_type_name = this->get_next_leader()->get_leader_type_name();

				engine_interface::get()->add_notification(std::format("{} Unavailable", leader_type_name), war_minister_portrait, std::format("Your Excellency, the {} {} has unfortunately decided to join {}, and is no longer available for recruitment.", string::lowered(leader_type_name), this->get_next_leader()->get_full_name(), this->get_next_leader()->get_game_data()->get_country()->get_game_data()->get_name()));
			}

			this->set_next_leader(nullptr);
		} else if (
			(this->get_next_leader()->get_obsolescence_technology() != nullptr && this->has_technology(this->get_next_leader()->get_obsolescence_technology()))
			|| this->get_best_military_unit_category_type(this->get_next_leader()->get_military_unit_category(), this->get_next_leader()->get_culture()) == nullptr
		) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *war_minister_portrait = this->get_war_minister_portrait();

				const std::string_view leader_type_name = this->get_next_leader()->get_leader_type_name();

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
		if (this->get_commodity_output(defines::get()->get_leader_commodity()).to_int() > 0 || this->get_stored_commodity(defines::get()->get_leader_commodity()) > 0) {
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
		if (character->get_role() != character_role::leader) {
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

		const military_unit_category leader_category = character->get_military_unit_category();

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

		int best_desire = -1;
		for (const auto &[category, leader] : potential_leader_map) {
			const military_unit_type *military_unit_type = this->get_best_military_unit_category_type(leader->get_military_unit_category(), leader->get_culture());
			assert_throw(military_unit_type != nullptr);
			const int leader_score = military_unit_type->get_score();

			assert_throw(leader_score >= 0);

			int desire = leader_score;

			for (const journal_entry *journal_entry : this->get_active_journal_entries()) {
				if (vector::contains(journal_entry->get_recruited_characters(), leader)) {
					desire += journal_entry::ai_leader_desire_modifier;
				}
			}

			assert_throw(desire >= 0);

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

void country_game_data::check_civilian_characters()
{
	//remove obsolete civilian characters
	const std::vector<const character *> civilian_characters = this->get_civilian_characters();
	for (const character *character : civilian_characters) {
		if (character->get_obsolescence_technology() != nullptr && this->has_technology(character->get_obsolescence_technology())) {
			if (this->country == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

				const std::string &civilian_unit_type_name = character->get_civilian_unit_type()->get_name();

				engine_interface::get()->add_notification(std::format("{} Retired", civilian_unit_type_name), interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the {} {} has decided to retire.", string::lowered(civilian_unit_type_name), character->get_full_name()));
			}

			assert_throw(character->get_game_data()->get_civilian_unit() != nullptr);
			character->get_game_data()->get_civilian_unit()->disband(true);
		}
	}
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

void country_game_data::do_sale(const metternich::country *other_country, const commodity *commodity, const int sold_quantity, const bool state_purchase)
{
	this->change_stored_commodity(commodity, -sold_quantity);

	const int price = game::get()->get_price(commodity);
	const int sale_income = price * sold_quantity;
	this->add_taxable_wealth(sale_income, income_transaction_type::tariff);
	this->country->get_turn_data()->add_income_transaction(income_transaction_type::sale, sale_income, commodity, sold_quantity, other_country != this->country ? other_country : nullptr);

	this->change_offer(commodity, -sold_quantity);

	country_game_data *other_country_game_data = other_country->get_game_data();

	if (state_purchase) {
		other_country_game_data->change_stored_commodity(commodity, sold_quantity);
		const int purchase_expense = other_country_game_data->get_inflated_value(price * sold_quantity);
		other_country_game_data->change_wealth(-purchase_expense);
		other_country->get_turn_data()->add_expense_transaction(expense_transaction_type::purchase, purchase_expense, commodity, sold_quantity, this->country);

		other_country_game_data->change_bid(commodity, -sold_quantity);
	}

	//improve relations between the two countries after they traded (even if it was not a state purchase)
	if (this->country != other_country) {
		this->change_base_opinion(other_country, 1);
		other_country_game_data->change_base_opinion(this->country, 1);
	}
}

void country_game_data::calculate_commodity_needs()
{
	this->commodity_needs.clear();
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

	if (phenotype == nullptr) {
		const std::vector<const metternich::phenotype *> weighted_phenotypes = this->get_weighted_phenotypes();
		if (weighted_phenotypes.empty()) {
			return false;
		}

		phenotype = vector::get_random(weighted_phenotypes);
	}

	assert_throw(phenotype != nullptr);

	auto civilian_unit = make_qunique<metternich::civilian_unit>(civilian_unit_type, this->country, phenotype);
	civilian_unit->set_tile_pos(tile_pos);

	this->add_civilian_unit(std::move(civilian_unit));

	return true;
}

void country_game_data::add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit)
{
	if (civilian_unit->get_character() != nullptr) {
		civilian_unit->get_character()->get_game_data()->set_country(this->country);
	}

	this->civilian_units.push_back(std::move(civilian_unit));
}

void country_game_data::remove_civilian_unit(civilian_unit *civilian_unit)
{
	assert_throw(civilian_unit != nullptr);

	if (civilian_unit->get_character() != nullptr) {
		assert_throw(civilian_unit->get_character()->get_game_data()->get_country() == this->country);
		civilian_unit->get_character()->get_game_data()->set_country(nullptr);
	}

	for (size_t i = 0; i < this->civilian_units.size(); ++i) {
		if (this->civilian_units[i].get() == civilian_unit) {
			this->civilian_units.erase(this->civilian_units.begin() + i);
			return;
		}
	}
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

			this->change_stored_commodity(commodity, -cost_change);
		}

		if (civilian_unit_type->get_wealth_cost() > 0) {
			const int wealth_cost_change = civilian_unit_type->get_wealth_cost() * change;
			this->change_wealth_inflated(-wealth_cost_change);
		}
	}
}

bool country_game_data::can_increase_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type) const
{
	//FIXME: check whether the country has a building capable of training the civilian unit type

	for (const auto &[commodity, cost] : civilian_unit_type->get_commodity_costs()) {
		assert_throw(commodity->is_storable());
		if (this->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	if (civilian_unit_type->get_wealth_cost() != 0 && this->get_wealth_with_credit() < this->get_inflated_value(civilian_unit_type->get_wealth_cost())) {
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

void country_game_data::add_military_unit(qunique_ptr<military_unit> &&military_unit)
{
	this->military_unit_names.insert(military_unit->get_name());
	this->military_units.push_back(std::move(military_unit));
}

void country_game_data::remove_military_unit(military_unit *military_unit)
{
	this->military_unit_names.erase(military_unit->get_name());

	for (size_t i = 0; i < this->military_units.size(); ++i) {
		if (this->military_units[i].get() == military_unit) {
			this->military_units.erase(this->military_units.begin() + i);
			return;
		}
	}
}

void country_game_data::add_army(qunique_ptr<army> &&army)
{
	this->armies.push_back(std::move(army));
}

void country_game_data::remove_army(army *army)
{
	for (size_t i = 0; i < this->armies.size(); ++i) {
		if (this->armies[i].get() == army) {
			this->armies.erase(this->armies.begin() + i);
			return;
		}
	}
}

void country_game_data::add_transporter(qunique_ptr<transporter> &&transporter)
{
	if (transporter->is_ship()) {
		this->change_sea_transport_capacity(transporter->get_cargo());
	} else {
		this->change_land_transport_capacity(transporter->get_cargo());
	}

	this->transporters.push_back(std::move(transporter));
}

void country_game_data::remove_transporter(transporter *transporter)
{
	if (transporter->is_ship()) {
		this->change_sea_transport_capacity(-transporter->get_cargo());
	} else {
		this->change_land_transport_capacity(-transporter->get_cargo());
	}

	for (size_t i = 0; i < this->transporters.size(); ++i) {
		if (this->transporters[i].get() == transporter) {
			this->transporters.erase(this->transporters.begin() + i);
			return;
		}
	}
}

const military_unit_type *country_game_data::get_best_military_unit_category_type(const military_unit_category category, const culture *culture) const
{
	const military_unit_type *best_type = nullptr;
	int best_score = -1;

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

		bool upgrade_is_available = false;
		for (const military_unit_type *upgrade : type->get_upgrades()) {
			if (culture->get_military_class_unit_type(upgrade->get_unit_class()) != upgrade) {
				continue;
			}

			if (upgrade->get_required_technology() != nullptr && !this->has_technology(upgrade->get_required_technology())) {
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

const military_unit_type *country_game_data::get_best_military_unit_category_type(const military_unit_category category) const
{
	return this->get_best_military_unit_category_type(category, this->country->get_culture());
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

		if (type->get_required_technology() != nullptr && !this->has_technology(type->get_required_technology())) {
			continue;
		}

		bool upgrade_is_available = false;
		for (const transporter_type *upgrade : type->get_upgrades()) {
			if (culture->get_transporter_class_type(upgrade->get_transporter_class()) != upgrade) {
				continue;
			}

			if (upgrade->get_required_technology() != nullptr && !this->has_technology(upgrade->get_required_technology())) {
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

void country_game_data::set_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_military_unit_type_stat_modifier(type, stat);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->military_unit_type_stat_modifiers[type].erase(stat);

		if (this->military_unit_type_stat_modifiers[type].empty()) {
			this->military_unit_type_stat_modifiers.erase(type);
		}
	} else {
		this->military_unit_type_stat_modifiers[type][stat] = value;
	}

	const centesimal_int difference = value - old_value;
	for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
		if (military_unit->get_type() != type) {
			continue;
		}

		military_unit->change_stat(stat, difference);
	}
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

void country_game_data::set_output_modifier(const centesimal_int &value)
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

	this->calculate_site_commodity_outputs();

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

	this->calculate_site_commodity_outputs();

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

void country_game_data::set_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
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

	this->calculate_site_commodity_output(commodity);
}

void country_game_data::set_capital_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
{
	if (value == this->get_capital_commodity_output_modifier(commodity)) {
		return;
	}

	if (value == 0) {
		this->capital_commodity_output_modifiers.erase(commodity);
	} else {
		this->capital_commodity_output_modifiers[commodity] = value;
	}

	if (this->get_capital() != nullptr) {
		this->get_capital()->get_game_data()->calculate_commodity_outputs();
	}
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

void country_game_data::change_improvement_commodity_bonus(const improvement *improvement, const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int &count = (this->improvement_commodity_bonuses[improvement][commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->improvement_commodity_bonuses[improvement].erase(commodity);

		if (this->improvement_commodity_bonuses[improvement].empty()) {
			this->improvement_commodity_bonuses.erase(improvement);
		}
	}

	for (const province *province : this->get_provinces()) {
		for (const QPoint &tile_pos : province->get_game_data()->get_resource_tiles()) {
			const tile *tile = map::get()->get_tile(tile_pos);
			const site *site = tile->get_site();

			if (site->get_game_data()->has_improvement(improvement)) {
				site->get_game_data()->change_base_commodity_output(commodity, change);
			}
		}
	}
}

void country_game_data::change_building_commodity_bonus(const building_type *building, const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->building_commodity_bonuses[building][commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->building_commodity_bonuses[building].erase(commodity);

		if (this->building_commodity_bonuses[building].empty()) {
			this->building_commodity_bonuses.erase(building);
		}
	}

	for (const province *province : this->get_provinces()) {
		for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
			if (!settlement->get_game_data()->is_built()) {
				continue;
			}

			if (settlement->get_game_data()->has_building(building)) {
				settlement->get_game_data()->change_base_commodity_output(commodity, centesimal_int(change));
			}
		}
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

void country_game_data::change_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->commodity_bonuses_per_population[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_bonuses_per_population.erase(commodity);
	}

	for (const province *province : this->get_provinces()) {
		for (const site *site : province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population() || !site->get_game_data()->is_built()) {
				continue;
			}

			site->get_game_data()->calculate_commodity_outputs();
		}
	}
}

void country_game_data::change_settlement_commodity_bonus(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->settlement_commodity_bonuses[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->settlement_commodity_bonuses.erase(commodity);
	}

	for (const province *province : this->get_provinces()) {
		for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
			if (!settlement->get_game_data()->is_built()) {
				continue;
			}

			settlement->get_game_data()->change_base_commodity_output(commodity, centesimal_int(change));
		}
	}
}

void country_game_data::change_capital_commodity_bonus(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->capital_commodity_bonuses[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->capital_commodity_bonuses.erase(commodity);
	}

	if (this->get_capital() != nullptr) {
		this->get_capital()->get_game_data()->calculate_commodity_outputs();
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

void country_game_data::set_technology_category_cost_modifier(const technology_category *category, const centesimal_int &value)
{
	if (value == this->get_technology_category_cost_modifier(category)) {
		return;
	}

	if (value == 0) {
		this->technology_category_cost_modifiers.erase(category);
	} else {
		this->technology_category_cost_modifiers[category] = value;
	}
}

void country_game_data::set_technology_subcategory_cost_modifier(const technology_subcategory *subcategory, const centesimal_int &value)
{
	if (value == this->get_technology_subcategory_cost_modifier(subcategory)) {
		return;
	}

	if (value == 0) {
		this->technology_subcategory_cost_modifiers.erase(subcategory);
	} else {
		this->technology_subcategory_cost_modifiers[subcategory] = value;
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

void country_game_data::set_population_type_militancy_modifier(const population_type *type, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_population_type_militancy_modifier(type);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->population_type_militancy_modifiers.erase(type);
	} else {
		this->population_type_militancy_modifiers[type] = value;
	}
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
		if (this->can_gain_technology(tile_resource->get_discovery_technology())) {
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
			if (this->can_gain_technology(tile_resource->get_discovery_technology())) {
				this->add_technology(tile_resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit technology_researched(tile_resource->get_discovery_technology());
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
		if (!tile->is_resource_discovered() && (tile_resource->get_required_technology() == nullptr || this->has_technology(tile_resource->get_required_technology()))) {
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

void country_game_data::set_free_warship_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_warship_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_warship_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_warship_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_ship()) {
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

void country_game_data::calculate_tile_transport_levels()
{
	this->clear_tile_transport_levels();

	const site *capital = this->get_capital();
	if (capital == nullptr) {
		return;
	}

	map::get()->calculate_tile_transport_level(capital->get_map_data()->get_tile_pos());

	//calculate transport levels beginning from sites with ports
	for (const province *province : this->get_provinces()) {
		for (const site *site : province->get_game_data()->get_sites()) {
			if (site == capital) {
				//already calculated
				continue;
			}

			if (site->get_game_data()->get_improvement(improvement_slot::port) == nullptr) {
				continue;
			}

			map::get()->calculate_tile_transport_level(site->get_map_data()->get_tile_pos());
		}
	}
}

void country_game_data::clear_tile_transport_levels()
{
	for (const province *province : this->get_provinces()) {
		for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
			map::get()->clear_tile_transport_level(settlement->get_map_data()->get_tile_pos());
		}
	}
}

}
