#include "metternich.h"

#include "country/country_economy.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_turn_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"
#include "economy/income_transaction_type.h"
#include "game/game.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
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
		for (const auto &[commodity, output] : this->get_commodity_outputs()) {
			if (!commodity->is_storable()) {
				assert_throw(output >= 0);
				continue;
			}

			this->change_stored_commodity(commodity, output.to_int());
		}

		//reduce inputs from the storage for the next turn (for production this turn it had already been subtracted)
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
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing production for country \"{}\".", this->country->get_identifier())));
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
					sold_quantity = std::min(sold_quantity, other_country_economy->get_wealth() / price);

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

QVariantList country_economy::get_resource_counts_qvariant_list() const
{
	return archimedes::map::to_value_sorted_qvariant_list(this->get_resource_counts());
}

QVariantList country_economy::get_vassal_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_vassal_resource_counts());
}

int country_economy::get_wealth() const
{
	return this->get_stored_commodity(defines::get()->get_wealth_commodity());
}

void country_economy::set_wealth(const int wealth)
{
	this->set_stored_commodity(defines::get()->get_wealth_commodity(), wealth);
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

	if (value <= 0) {
		this->stored_commodities.erase(commodity);
	} else {
		this->stored_commodities[commodity] = value;
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

	if (commodity->is_storable() && change_input_storage) {
		this->change_stored_commodity(commodity, old_input.to_int());
	}

	const centesimal_int new_input = (this->commodity_inputs[commodity] += change);

	assert_throw(new_input >= 0);

	if (new_input == 0) {
		this->commodity_inputs.erase(commodity);
	}

	if (commodity->is_storable() && change_input_storage) {
		this->change_stored_commodity(commodity, -new_input.to_int());
	}

	if (game::get()->is_running()) {
		emit commodity_inputs_changed();
	}
}

bool country_economy::can_change_commodity_input(const commodity *commodity, const centesimal_int &change) const
{
	if (change <= 0) {
		return true;
	}

	if (commodity->is_storable()) {
		const centesimal_int current_commodity_input = this->get_commodity_input(commodity);
		const int storage_change = (current_commodity_input + change).to_int() - current_commodity_input.to_int();

		if (this->get_stored_commodity(commodity) < storage_change) {
			return false;
		}
	} else {
		//for non-storable commodities, like Labor, the commodity output is used directly instead of storage
		if (this->get_net_commodity_output(commodity) < change) {
			return false;
		}
	}

	return true;
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

	const centesimal_int &new_output = (this->commodity_outputs[commodity] += change);

	assert_throw(new_output >= 0);

	if (new_output == 0) {
		this->commodity_outputs.erase(commodity);
	}

	if (game::get()->is_running()) {
		emit commodity_outputs_changed();
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

	return false;
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
		const int purchase_expense = price * sold_quantity;
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

	this->output_modifier = value;

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

	this->industrial_output_modifier = value;

	if (game::get()->is_running()) {
		emit industrial_output_modifier_changed();
	}
}

void country_economy::set_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
{
	if (value == this->get_commodity_output_modifier(commodity)) {
		return;
	}

	if (value == 0) {
		this->commodity_output_modifiers.erase(commodity);
	} else {
		this->commodity_output_modifiers[commodity] = value;
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

	this->throughput_modifier = value;

	if (game::get()->is_running()) {
		emit throughput_modifier_changed();
	}
}

void country_economy::set_commodity_throughput_modifier(const commodity *commodity, const int value)
{
	if (value == this->get_commodity_throughput_modifier(commodity)) {
		return;
	}

	if (value == 0) {
		this->commodity_throughput_modifiers.erase(commodity);
	} else {
		this->commodity_throughput_modifiers[commodity] = value;
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
