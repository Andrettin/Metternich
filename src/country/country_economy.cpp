#include "metternich.h"

#include "country/country_economy.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_turn_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"
#include "economy/income_transaction_type.h"
#include "economy/production_type.h"
#include "game/game.h"
#include "infrastructure/country_building_slot.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "population/education_type.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/profession.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace metternich {

country_economy::country_economy(const metternich::country *country, const country_game_data *game_data)
	: country(country)
{
	connect(game_data, &country_game_data::provinces_changed, this, &country_economy::resource_counts_changed);
	connect(game_data, &country_game_data::diplomacy_states_changed, this, &country_economy::vassal_resource_counts_changed);

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
}

country_economy::~country_economy()
{
}

country_game_data *country_economy::get_game_data() const
{
	return this->country->get_game_data();
}

void country_economy::do_production()
{
	try {
		//FIXME: add preference for production being automatically assigned for person players
		if (this->get_game_data()->is_ai()) {
			this->assign_production();
		}

		for (const auto &[commodity, output] : this->get_commodity_outputs()) {
			if (!commodity->is_storable()) {
				assert_throw(output >= 0);
				continue;
			}

			this->change_stored_commodity(commodity, output.to_int());
		}

		std::vector<employment_location *> changed_employment_locations;

		//decrease consumption of commodities for which we no longer have enough in storage
		while (this->get_wealth_income() < 0 && (this->get_wealth_income() * -1) > this->get_wealth_with_credit()) {
			employment_location *affected_employment_location = this->decrease_wealth_consumption(false);
			if (affected_employment_location != nullptr && !vector::contains(changed_employment_locations, affected_employment_location)) {
				changed_employment_locations.push_back(affected_employment_location);
			}
		}

		const std::vector<const commodity *> input_commodities = archimedes::map::get_keys(this->get_commodity_inputs());

		for (const commodity *commodity : input_commodities) {
			if (!commodity->is_storable() || commodity->is_negative_allowed()) {
				continue;
			}

			while (this->get_commodity_input(commodity).to_int() > this->get_stored_commodity(commodity)) {
				employment_location *affected_employment_location = this->decrease_commodity_consumption(commodity, false);
				if (affected_employment_location != nullptr && !vector::contains(changed_employment_locations, affected_employment_location)) {
					changed_employment_locations.push_back(affected_employment_location);
				}
			}
		}

		//reduce inputs from the storage for the next turn (for production this turn it had already been subtracted)
		if (this->get_wealth_income() != 0) {
			this->change_wealth(this->get_wealth_income());
		}

		for (const auto &[commodity, input] : this->get_commodity_inputs()) {
			try {
				const int input_int = input.to_int();

				if (!commodity->is_storable()) {
					const int output = this->get_commodity_output(commodity).to_int();
					if (input_int > output) {
						throw std::runtime_error(std::format("Input for non-storable commodity \"{}\" ({}) is greater than its output ({}).", commodity->get_identifier(), input_int, output));
					}
					continue;
				}

				this->change_stored_commodity(commodity, -input_int);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Error processing input storage reduction for commodity \"" + commodity->get_identifier() + "\"."));
			}
		}

		for (employment_location *employment_location : changed_employment_locations) {
			//check if employment has become superfluous due to decrease
			employment_location->check_superfluous_employment();
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing production for country \"{}\".", this->country->get_identifier())));
	}
}

void country_economy::do_everyday_consumption()
{
	if (this->get_game_data()->get_population_units().empty()) {
		return;
	}

	const std::vector<population_unit *> population_units = vector::shuffled(this->get_game_data()->get_population_units());

	for (population_unit *population_unit : population_units) {
		population_unit->set_everyday_consumption_fulfilled(true);
	}

	const int inflated_everyday_wealth_consumption = this->get_inflated_value(this->get_everyday_wealth_consumption());

	if (inflated_everyday_wealth_consumption > 0) {
		const int effective_consumption = std::max(0, std::min(inflated_everyday_wealth_consumption, this->get_wealth_with_credit()));

		if (effective_consumption > 0) {
			this->change_wealth(-effective_consumption);

			for (const auto &[population_type, count] : this->get_game_data()->get_population()->get_type_counts()) {
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

	for (const province *province : this->get_game_data()->get_provinces()) {
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

void country_economy::do_luxury_consumption()
{
	if (this->get_game_data()->get_population_units().empty()) {
		return;
	}

	const std::vector<population_unit *> population_units = vector::shuffled(this->get_game_data()->get_population_units());

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

	for (const province *province : this->get_game_data()->get_provinces()) {
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

void country_economy::do_trade(country_map<commodity_map<int>> &country_luxury_demands)
{
	try {
		if (this->get_game_data()->is_under_anarchy()) {
			return;
		}

		//get the known countries and sort them by priority
		std::vector<const metternich::country *> countries = container::to_vector(this->get_game_data()->get_known_countries());
		std::sort(countries.begin(), countries.end(), [&](const metternich::country *lhs, const metternich::country *rhs) {
			if (this->get_game_data()->is_vassal_of(lhs) != this->get_game_data()->is_vassal_of(rhs)) {
				return this->get_game_data()->is_vassal_of(lhs);
			}

			if (this->get_game_data()->is_any_vassal_of(lhs) != this->get_game_data()->is_any_vassal_of(rhs)) {
				return this->get_game_data()->is_any_vassal_of(lhs);
			}

			if (defines::get()->get_prestige_commodity()->is_enabled()) {
				//give trade priority by opinion-weighted prestige
				const int lhs_opinion_weighted_prestige = this->get_game_data()->get_opinion_weighted_prestige_for(lhs);
				const int rhs_opinion_weighted_prestige = this->get_game_data()->get_opinion_weighted_prestige_for(rhs);

				if (lhs_opinion_weighted_prestige != rhs_opinion_weighted_prestige) {
					return lhs_opinion_weighted_prestige > rhs_opinion_weighted_prestige;
				}
			} else {
				//give trade priority by opinion
				const int lhs_opinion = this->get_game_data()->get_opinion_of(lhs);
				const int rhs_opinion = this->get_game_data()->get_opinion_of(rhs);

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
				country_economy *other_country_economy = other_country->get_economy();

				const int bid = other_country_economy->get_bid(commodity);
				if (bid != 0) {
					int sold_quantity = std::min(offer, bid);
					sold_quantity = std::min(sold_quantity, other_country_economy->get_wealth_with_credit() / price);

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

void country_economy::do_inflation()
{
	try {
		if (this->get_game_data()->is_under_anarchy()) {
			return;
		}

		this->country->get_turn_data()->calculate_inflation();
		this->change_inflation(this->country->get_turn_data()->get_total_inflation_change());

		this->change_inflation(this->get_inflation_change());
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing inflation for country \"{}\".", this->country->get_identifier())));
	}
}

QVariantList country_economy::get_resource_counts_qvariant_list() const
{
	return archimedes::map::to_value_sorted_qvariant_list(this->get_resource_counts());
}

QVariantList country_economy::get_vassal_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_vassal_resource_counts());
}

void country_economy::add_taxable_wealth(const int taxable_wealth, const income_transaction_type tax_income_type)
{
	assert_throw(taxable_wealth >= 0);
	assert_throw(tax_income_type == income_transaction_type::tariff || tax_income_type == income_transaction_type::treasure_fleet);

	if (taxable_wealth == 0) {
		return;
	}

	if (this->get_game_data()->get_overlord() == nullptr) {
		this->change_wealth(taxable_wealth);
		return;
	}

	const int tax = taxable_wealth * country_game_data::vassal_tax_rate / 100;
	const int taxed_wealth = taxable_wealth - tax;

	this->get_game_data()->get_overlord()->get_economy()->add_taxable_wealth(tax, tax_income_type);

	this->change_wealth(taxed_wealth);

	if (tax != 0) {
		this->get_game_data()->get_overlord()->get_turn_data()->add_income_transaction(tax_income_type, tax, nullptr, 0, this->country);
		this->country->get_turn_data()->add_expense_transaction(expense_transaction_type::tax, tax, nullptr, 0, this->get_game_data()->get_overlord());
	}
}

void country_economy::set_wealth_income(const int income)
{
	if (income == this->get_wealth_income()) {
		return;
	}

	this->get_game_data()->change_economic_score(-this->get_wealth_income());

	this->wealth_income = income;

	this->get_game_data()->change_economic_score(this->get_wealth_income());

	emit wealth_income_changed();
}

void country_economy::set_inflation(const centesimal_int &inflation)
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

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
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

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
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

void country_economy::set_inflation_change(const centesimal_int &inflation_change)
{
	if (inflation_change == this->get_inflation_change()) {
		return;
	}

	this->inflation_change = inflation_change;
}

QVariantList country_economy::get_available_commodities_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_commodities());
}

QVariantList country_economy::get_tradeable_commodities_qvariant_list() const
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

QVariantList country_economy::get_stored_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_stored_commodities());
}

void country_economy::set_stored_commodity(const commodity *commodity, const int value)
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
		this->get_game_data()->change_score(-this->get_stored_commodity(commodity));
	}

	if (value <= 0) {
		this->stored_commodities.erase(commodity);
	} else {
		this->stored_commodities[commodity] = value;
	}

	if (commodity == defines::get()->get_prestige_commodity()) {
		this->get_game_data()->change_score(value);
	}

	if (this->get_offer(commodity) > value) {
		this->set_offer(commodity, value);
	}

	if (game::get()->is_running()) {
		emit stored_commodities_changed();
	}
}

int country_economy::get_stored_food() const
{
	int stored_food = 0;

	for (const auto &[commodity, quantity] : this->get_stored_commodities()) {
		if (commodity->is_food()) {
			stored_food += quantity;
		}
	}

	return stored_food;
}

void country_economy::set_storage_capacity(const int capacity)
{
	if (capacity == this->get_storage_capacity()) {
		return;
	}

	this->storage_capacity = capacity;

	if (game::get()->is_running()) {
		emit storage_capacity_changed();
	}
}

QVariantList country_economy::get_commodity_inputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_inputs());
}

int country_economy::get_commodity_input(const QString &commodity_identifier) const
{
	return this->get_commodity_input(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_economy::change_commodity_input(const commodity *commodity, const centesimal_int &change, const bool change_input_storage)
{
	if (change == 0) {
		return;
	}

	const centesimal_int old_input = this->get_commodity_input(commodity);

	if (commodity->get_base_price() != 0) {
		this->get_game_data()->change_economic_score(old_input.to_int() * commodity->get_base_price());
	}

	if (commodity->is_storable() && change_input_storage) {
		this->change_stored_commodity(commodity, old_input.to_int());
	}

	const centesimal_int new_input = (this->commodity_inputs[commodity] += change);

	assert_throw(new_input >= 0);

	if (new_input == 0) {
		this->commodity_inputs.erase(commodity);
	}

	if (commodity->get_base_price() != 0) {
		this->get_game_data()->change_economic_score(-new_input.to_int() * commodity->get_base_price());
	}

	if (commodity->is_storable() && change_input_storage) {
		this->change_stored_commodity(commodity, -new_input.to_int());
	}

	if (game::get()->is_running()) {
		emit commodity_inputs_changed();
	}
}

QVariantList country_economy::get_transportable_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_transportable_commodity_outputs());
}

int country_economy::get_transportable_commodity_output(const QString &commodity_identifier) const
{
	return this->get_transportable_commodity_output(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_economy::change_transportable_commodity_output(const commodity *commodity, const centesimal_int &change)
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

QVariantList country_economy::get_transported_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_transported_commodity_outputs());
}

void country_economy::change_transported_commodity_output(const commodity *commodity, const int change)
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

QVariantList country_economy::get_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_outputs());
}

int country_economy::get_commodity_output(const QString &commodity_identifier) const
{
	return this->get_commodity_output(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_economy::change_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int old_output = this->get_commodity_output(commodity);

	if (commodity->get_base_price() != 0 || commodity->get_wealth_value() != 0) {
		const int commodity_value = commodity->get_base_price() != 0 ? commodity->get_base_price() : commodity->get_wealth_value();
		this->get_game_data()->change_economic_score(-old_output.to_int() * commodity_value);
	}

	const centesimal_int &new_output = (this->commodity_outputs[commodity] += change);

	assert_throw(new_output >= 0);

	if (new_output == 0) {
		this->commodity_outputs.erase(commodity);
	}

	if (commodity->get_base_price() != 0 || commodity->get_wealth_value() != 0) {
		const int commodity_value = commodity->get_base_price() != 0 ? commodity->get_base_price() : commodity->get_wealth_value();
		this->get_game_data()->change_economic_score(new_output.to_int() * commodity_value);
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

void country_economy::calculate_site_commodity_outputs()
{
	for (const province *province : this->get_game_data()->get_provinces()) {
		province->get_game_data()->calculate_site_commodity_outputs();
	}
}

void country_economy::calculate_site_commodity_output(const commodity *commodity)
{
	for (const province *province : this->get_game_data()->get_provinces()) {
		province->get_game_data()->calculate_site_commodity_output(commodity);
	}
}

int country_economy::get_food_output() const
{
	int food_output = 0;

	for (const auto &[commodity, output] : this->get_commodity_outputs()) {
		if (commodity->is_food()) {
			food_output += output.to_int();
		}
	}

	return food_output;
}

void country_economy::change_everyday_wealth_consumption(const int change)
{
	if (change == 0) {
		return;
	}

	this->everyday_wealth_consumption += change;

	if (game::get()->is_running()) {
		emit everyday_wealth_consumption_changed();
	}
}

QVariantList country_economy::get_everyday_consumption_qvariant_list() const
{
	commodity_map<int> int_everyday_consumption;

	for (const auto &[commodity, consumption] : this->get_everyday_consumption()) {
		int_everyday_consumption[commodity] = consumption.to_int();
	}

	return archimedes::map::to_qvariant_list(int_everyday_consumption);
}

int country_economy::get_everyday_consumption(const QString &commodity_identifier) const
{
	return this->get_everyday_consumption(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_economy::change_everyday_consumption(const commodity *commodity, const centesimal_int &change)
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

QVariantList country_economy::get_luxury_consumption_qvariant_list() const
{
	commodity_map<int> int_luxury_consumption;

	for (const auto &[commodity, consumption] : this->get_luxury_consumption()) {
		int_luxury_consumption[commodity] = consumption.to_int();
	}

	return archimedes::map::to_qvariant_list(int_luxury_consumption);
}

int country_economy::get_luxury_consumption(const QString &commodity_identifier) const
{
	return this->get_luxury_consumption(commodity::get(commodity_identifier.toStdString())).to_int();
}

void country_economy::change_luxury_consumption(const commodity *commodity, const centesimal_int &change)
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

void country_economy::change_commodity_demand(const commodity *commodity, const decimillesimal_int &change)
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

void country_economy::assign_production()
{
	bool changed = true;

	while (changed) {
		changed = false;

		for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
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

employment_location *country_economy::decrease_wealth_consumption(const bool restore_inputs)
{
	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
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
			return nullptr;
		}

		for (const education_type *education_type : building_slot->get_available_education_types()) {
			if (education_type->get_input_wealth() == 0) {
				continue;
			}

			if (!building_slot->can_decrease_education(education_type)) {
				continue;
			}

			building_slot->decrease_education(education_type, restore_inputs);
			return nullptr;
		}
	}

	const std::vector<const province *> provinces = vector::shuffled(this->get_game_data()->get_provinces());
	for (const province *province : provinces) {
		for (employment_location *employment_location : province->get_game_data()->get_employment_locations()) {
			if (employment_location->get_employee_count() == 0) {
				continue;
			}

			for (const profession *profession : employment_location->get_employment_professions()) {
				if (profession->get_input_wealth() == 0) {
					continue;
				}

				employment_location->decrease_employment(profession, restore_inputs, std::nullopt);
				return employment_location;
			}
		}
	}

	assert_throw(false);
	return nullptr;
}

employment_location *country_economy::decrease_commodity_consumption(const commodity *commodity, const bool restore_inputs)
{
	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
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
			return nullptr;
		}

		for (const education_type *education_type : building_slot->get_available_education_types()) {
			if (!education_type->get_input_commodities().contains(commodity)) {
				continue;
			}

			if (!building_slot->can_decrease_education(education_type)) {
				continue;
			}

			building_slot->decrease_education(education_type, restore_inputs);
			return nullptr;
		}
	}

	const std::vector<const province *> provinces = vector::shuffled(this->get_game_data()->get_provinces());
	for (const province *province : provinces) {
		for (employment_location *employment_location : province->get_game_data()->get_employment_locations()) {
			if (employment_location->get_employee_count() == 0) {
				continue;
			}

			for (const profession *profession : employment_location->get_employment_professions()) {
				if (!profession->get_input_commodities().contains(commodity)) {
					continue;
				}

				employment_location->decrease_employment(profession, restore_inputs, std::nullopt);
				return employment_location;
			}
		}
	}

	assert_throw(false);
	return nullptr;
}

bool country_economy::produces_commodity(const commodity *commodity) const
{
	if (this->get_commodity_output(commodity).to_int() > 0) {
		return true;
	}

	for (const province *province : this->get_game_data()->get_provinces()) {
		if (province->get_game_data()->produces_commodity(commodity)) {
			return true;
		}
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() == commodity && building_slot->get_production_type_output(production_type).to_int() > 0) {
				return true;
			}
		}
	}

	return false;
}

void country_economy::set_land_transport_capacity(const int capacity)
{
	if (capacity == this->get_land_transport_capacity()) {
		return;
	}

	this->land_transport_capacity = capacity;

	if (game::get()->is_running()) {
		emit land_transport_capacity_changed();
	}
}

void country_economy::set_sea_transport_capacity(const int capacity)
{
	if (capacity == this->get_sea_transport_capacity()) {
		return;
	}

	this->sea_transport_capacity = capacity;

	if (game::get()->is_running()) {
		emit sea_transport_capacity_changed();
	}
}

void country_economy::assign_transport_orders()
{
	if (this->get_game_data()->is_under_anarchy()) {
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

QVariantList country_economy::get_bids_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_bids());
}

void country_economy::set_bid(const commodity *commodity, const int value)
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

QVariantList country_economy::get_offers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_offers());
}

void country_economy::set_offer(const commodity *commodity, const int value)
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

void country_economy::do_sale(const metternich::country *other_country, const commodity *commodity, const int sold_quantity, const bool state_purchase)
{
	this->change_stored_commodity(commodity, -sold_quantity);

	const int price = game::get()->get_price(commodity);
	const int sale_income = price * sold_quantity;
	this->add_taxable_wealth(sale_income, income_transaction_type::tariff);
	this->country->get_turn_data()->add_income_transaction(income_transaction_type::sale, sale_income, commodity, sold_quantity, other_country != this->country ? other_country : nullptr);

	this->change_offer(commodity, -sold_quantity);

	country_game_data *other_country_game_data = other_country->get_game_data();
	country_economy *other_country_economy = other_country->get_economy();

	if (state_purchase) {
		other_country_economy->change_stored_commodity(commodity, sold_quantity);
		const int purchase_expense = other_country_economy->get_inflated_value(price * sold_quantity);
		other_country_economy->change_wealth(-purchase_expense);
		other_country->get_turn_data()->add_expense_transaction(expense_transaction_type::purchase, purchase_expense, commodity, sold_quantity, this->country);

		other_country_economy->change_bid(commodity, -sold_quantity);
	}

	//improve relations between the two countries after they traded (even if it was not a state purchase)
	if (this->country != other_country) {
		this->get_game_data()->change_base_opinion(other_country, 1);
		other_country_game_data->change_base_opinion(this->country, 1);
	}
}

void country_economy::calculate_commodity_needs()
{
	this->commodity_needs.clear();
}

void country_economy::set_output_modifier(const centesimal_int &value)
{
	if (value == this->get_output_modifier()) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			this->change_commodity_output(production_type->get_output_commodity(), -building_slot->get_production_type_output(production_type));
		}
	}

	this->output_modifier = value;

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			this->change_commodity_output(production_type->get_output_commodity(), building_slot->get_production_type_output(production_type));
		}
	}

	this->calculate_site_commodity_outputs();

	if (game::get()->is_running()) {
		emit output_modifier_changed();
	}
}

void country_economy::set_resource_output_modifier(const int value)
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

void country_economy::set_industrial_output_modifier(const int value)
{
	if (value == this->get_industrial_output_modifier()) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (!production_type->is_industrial()) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), -building_slot->get_production_type_output(production_type));
		}
	}

	this->industrial_output_modifier = value;

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
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

void country_economy::set_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
{
	if (value == this->get_commodity_output_modifier(commodity)) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
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

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), building_slot->get_production_type_output(production_type));
		}
	}

	this->calculate_site_commodity_output(commodity);
}

void country_economy::set_capital_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
{
	if (value == this->get_capital_commodity_output_modifier(commodity)) {
		return;
	}

	if (value == 0) {
		this->capital_commodity_output_modifiers.erase(commodity);
	} else {
		this->capital_commodity_output_modifiers[commodity] = value;
	}

	if (this->get_game_data()->get_capital() != nullptr) {
		this->get_game_data()->get_capital()->get_game_data()->calculate_commodity_outputs();
	}
}

void country_economy::set_throughput_modifier(const int value)
{
	if (value == this->get_throughput_modifier()) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				this->change_commodity_input(input_commodity, -centesimal_int(input_value), true);
			}
		}
	}

	this->throughput_modifier = value;

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				this->change_commodity_input(input_commodity, centesimal_int(input_value), true);
			}
		}
	}

	if (game::get()->is_running()) {
		emit throughput_modifier_changed();
	}
}

void country_economy::set_commodity_throughput_modifier(const commodity *commodity, const int value)
{
	if (value == this->get_commodity_throughput_modifier(commodity)) {
		return;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				this->change_commodity_input(input_commodity, -centesimal_int(input_value), true);
			}
		}
	}

	if (value == 0) {
		this->commodity_throughput_modifiers.erase(commodity);
	} else {
		this->commodity_throughput_modifiers[commodity] = value;
	}

	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			const commodity_map<int> inputs = building_slot->get_production_type_inputs(production_type);
			for (const auto &[input_commodity, input_value] : inputs) {
				this->change_commodity_input(input_commodity, centesimal_int(input_value), true);
			}
		}
	}
}

void country_economy::change_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity, const int change)
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

	for (const province *province : this->get_game_data()->get_provinces()) {
		province->get_game_data()->change_improved_resource_commodity_bonus(resource, commodity, change);
	}
}

void country_economy::change_improvement_commodity_bonus(const improvement *improvement, const commodity *commodity, const centesimal_int &change)
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

	for (const province *province : this->get_game_data()->get_provinces()) {
		for (const QPoint &tile_pos : province->get_game_data()->get_resource_tiles()) {
			const tile *tile = map::get()->get_tile(tile_pos);
			const site *site = tile->get_site();

			if (site->get_game_data()->has_improvement(improvement)) {
				site->get_game_data()->change_base_commodity_output(commodity, change);
			}
		}
	}
}

void country_economy::change_building_commodity_bonus(const building_type *building, const commodity *commodity, const int change)
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

	for (const province *province : this->get_game_data()->get_provinces()) {
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

void country_economy::change_profession_commodity_bonus(const profession *profession, const commodity *commodity, const decimillesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const decimillesimal_int &count = (this->profession_commodity_bonuses[profession][commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->profession_commodity_bonuses[profession].erase(commodity);

		if (this->profession_commodity_bonuses[profession].empty()) {
			this->profession_commodity_bonuses.erase(profession);
		}
	}

	for (const province *province : this->get_game_data()->get_provinces()) {
		for (employment_location *employment_location : province->get_game_data()->get_employment_locations()) {
			if (employment_location->get_employee_count() == 0) {
				continue;
			}

			if (!vector::contains(employment_location->get_employment_professions(), profession)) {
				continue;
			}

			employment_location->calculate_total_employee_commodity_outputs();
		}
	}
}

void country_economy::set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
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

	for (const province *province : this->get_game_data()->get_provinces()) {
		province->get_game_data()->change_commodity_bonus_for_tile_threshold(commodity, threshold, value - old_value);
	}
}

void country_economy::change_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->commodity_bonuses_per_population[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_bonuses_per_population.erase(commodity);
	}

	for (const province *province : this->get_game_data()->get_provinces()) {
		for (const site *site : province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population() || !site->get_game_data()->is_built()) {
				continue;
			}

			site->get_game_data()->calculate_commodity_outputs();
		}
	}
}

void country_economy::change_settlement_commodity_bonus(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->settlement_commodity_bonuses[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->settlement_commodity_bonuses.erase(commodity);
	}

	for (const province *province : this->get_game_data()->get_provinces()) {
		for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
			if (!settlement->get_game_data()->is_built()) {
				continue;
			}

			settlement->get_game_data()->change_base_commodity_output(commodity, centesimal_int(change));
		}
	}
}

void country_economy::change_capital_commodity_bonus(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->capital_commodity_bonuses[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->capital_commodity_bonuses.erase(commodity);
	}

	if (this->get_game_data()->get_capital() != nullptr) {
		this->get_game_data()->get_capital()->get_game_data()->calculate_commodity_outputs();
	}
}

void country_economy::change_capital_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int count = (this->capital_commodity_bonuses_per_population[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->capital_commodity_bonuses_per_population.erase(commodity);
	}

	if (this->get_game_data()->get_capital() != nullptr) {
		this->get_game_data()->get_capital()->get_game_data()->calculate_commodity_outputs();
	}
}

}
