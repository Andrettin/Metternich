#include "metternich.h"

#include "domain/domain_economy.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/defines.h"
#include "domain/country_turn_data.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "domain/subject_type.h"
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
#include "population/population_strata.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

domain_economy::domain_economy(const metternich::domain *domain, const domain_game_data *game_data)
	: domain(domain)
{
	connect(game_data, &domain_game_data::provinces_changed, this, &domain_economy::resource_counts_changed);
	connect(game_data, &domain_game_data::diplomacy_states_changed, this, &domain_economy::vassal_resource_counts_changed);

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

domain_economy::~domain_economy()
{
}

void domain_economy::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "storage_capacity") {
		this->storage_capacity = std::stoll(value);
	} else {
		throw std::runtime_error(std::format("Invalid domain government property: \"{}\".", key));
	}
}

void domain_economy::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "stored_commodities") {
		scope.for_each_property([this](const gsml_property &property) {
			this->stored_commodities[commodity::get(property.get_key())] = std::stoll(property.get_value());
		});
	} else if (tag == "population_strata_tax_rates") {
		scope.for_each_property([this](const gsml_property &property) {
			this->population_strata_tax_rates[magic_enum::enum_cast<population_strata>(property.get_key()).value()] = std::stoi(property.get_value());
		});
	} else {
		throw std::runtime_error(std::format("Invalid domain government scope: \"{}\".", tag));
	}
}

gsml_data domain_economy::to_gsml_data() const
{
	gsml_data data("economy");

	data.add_property("storage_capacity", std::to_string(this->get_storage_capacity()));

	if (!this->get_stored_commodities().empty()) {
		gsml_data stored_commodities_data("stored_commodities");
		for (const auto &[commodity, quantity] : this->get_stored_commodities()) {
			stored_commodities_data.add_property(commodity->get_identifier(), std::to_string(quantity));
		}
		data.add_child(std::move(stored_commodities_data));
	}

	if (!this->population_strata_tax_rates.empty()) {
		gsml_data population_strata_tax_rates_data("population_strata_tax_rates");
		for (const auto &[strata, tax_rate] : this->population_strata_tax_rates) {
			population_strata_tax_rates_data.add_property(std::string(magic_enum::enum_name(strata)), std::to_string(tax_rate));
		}
		data.add_child(std::move(population_strata_tax_rates_data));
	}

	return data;
}

domain_game_data *domain_economy::get_game_data() const
{
	return this->domain->get_game_data();
}

void domain_economy::do_production()
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
		std::throw_with_nested(std::runtime_error(std::format("Error doing production for country \"{}\".", this->domain->get_identifier())));
	}
}

void domain_economy::do_trade(domain_map<commodity_map<int>> &country_luxury_demands)
{
	try {
		if (this->get_game_data()->is_under_anarchy()) {
			return;
		}

		//get the known countries and sort them by priority
		std::vector<const metternich::domain *> countries = container::to_vector(this->get_game_data()->get_known_countries());
		std::sort(countries.begin(), countries.end(), [&](const metternich::domain *lhs, const metternich::domain *rhs) {
			if (this->get_game_data()->is_vassal_of(lhs) != this->get_game_data()->is_vassal_of(rhs)) {
				return this->get_game_data()->is_vassal_of(lhs);
			}

			if (this->get_game_data()->is_any_vassal_of(lhs) != this->get_game_data()->is_any_vassal_of(rhs)) {
				return this->get_game_data()->is_any_vassal_of(lhs);
			}

			//give trade priority by opinion
			const int lhs_opinion = this->get_game_data()->get_opinion_of(lhs);
			const int rhs_opinion = this->get_game_data()->get_opinion_of(rhs);

			if (lhs_opinion != rhs_opinion) {
				return lhs_opinion > rhs_opinion;
			}

			return lhs->get_identifier() < rhs->get_identifier();
		});

		commodity_map<int> offers = this->get_offers();
		for (auto &[commodity, offer] : offers) {
			const int price = game::get()->get_price(commodity);

			for (const metternich::domain *other_domain : countries) {
				domain_economy *other_domain_economy = other_domain->get_economy();

				const int bid = other_domain_economy->get_bid(commodity);
				if (bid != 0) {
					int sold_quantity = std::min(offer, bid);
					sold_quantity = std::min(sold_quantity, other_domain_economy->get_wealth() / price);

					if (sold_quantity > 0) {
						this->do_sale(other_domain, commodity, sold_quantity, true);

						offer -= sold_quantity;

						if (offer == 0) {
							break;
						}
					}
				}

				int &demand = country_luxury_demands[other_domain][commodity];
				if (demand > 0) {
					const int sold_quantity = std::min(offer, demand);

					if (sold_quantity > 0) {
						this->do_sale(domain, commodity, sold_quantity, false);

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
		std::throw_with_nested(std::runtime_error(std::format("Error doing trade for domain \"{}\".", this->domain->get_identifier())));
	}
}

QVariantList domain_economy::get_resource_counts_qvariant_list() const
{
	return archimedes::map::to_value_sorted_qvariant_list(this->get_resource_counts());
}

QVariantList domain_economy::get_vassal_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_vassal_resource_counts());
}

int domain_economy::get_wealth() const
{
	return this->get_stored_commodity(defines::get()->get_wealth_commodity());
}

void domain_economy::set_wealth(const int wealth)
{
	this->set_stored_commodity(defines::get()->get_wealth_commodity(), wealth);
}

QVariantList domain_economy::get_available_commodities_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_commodities());
}

QVariantList domain_economy::get_tradeable_commodities_qvariant_list() const
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

QVariantList domain_economy::get_stored_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_stored_commodities());
}

void domain_economy::set_stored_commodity(const commodity *commodity, const int64_t value)
{
	if (!commodity->is_enabled()) {
		return;
	}

	if (value == this->get_stored_commodity(commodity)) {
		return;
	}

	if (value < 0 && !commodity->is_negative_allowed()) {
		throw std::runtime_error("Tried to set the storage of commodity \"" + commodity->get_identifier() + "\" for country \"" + this->domain->get_identifier() + "\" to a negative number.");
	}

	if (commodity->is_convertible_to_wealth()) {
		assert_throw(value > 0);
		const int64_t wealth_conversion_income = commodity->get_wealth_value() * value;
		const int64_t gained_wealth = this->add_population_wealth(wealth_conversion_income);
		this->add_tributable_commodity(defines::get()->get_wealth_commodity(), gained_wealth, income_transaction_type::treasure_fleet);
		this->domain->get_turn_data()->add_income_transaction(income_transaction_type::liquidated_riches, gained_wealth, commodity, value);
		return;
	}

	if (!commodity->is_abstract()) {
		const int64_t storage_capacity = this->get_storage_capacity_for_commodity(commodity);
		if (value > storage_capacity) {
			this->set_stored_commodity(commodity, storage_capacity);
			return;
		}
	}

	if (value <= 0) {
		this->stored_commodities.erase(commodity);
	} else {
		this->stored_commodities[commodity] = value;
	}

	if (this->get_offer(commodity) > value) {
		this->set_offer(commodity, value);
	}

	if (commodity == defines::get()->get_wealth_commodity() && this->domain->get_government()->get_ruler() != nullptr) {
		emit this->domain->get_government()->get_ruler()->get_game_data()->wealth_changed();
	}

	if (game::get()->is_running()) {
		emit stored_commodities_changed();
	}
}

void domain_economy::add_tributable_commodity(const commodity *commodity, const int tributable_quantity, const income_transaction_type tribute_income_type)
{
	assert_throw(commodity != nullptr);
	assert_throw(tributable_quantity >= 0);
	assert_throw(tribute_income_type == income_transaction_type::tariff || tribute_income_type == income_transaction_type::treasure_fleet || tribute_income_type == income_transaction_type::tribute);

	if (tributable_quantity == 0) {
		return;
	}

	if (this->get_game_data()->get_overlord() == nullptr) {
		this->change_stored_commodity(commodity, tributable_quantity);
		return;
	}

	assert_throw(this->get_game_data()->get_subject_type() != nullptr);

	int tribute_rate = 0;
	if (commodity == defines::get()->get_wealth_commodity()) {
		tribute_rate = this->get_game_data()->get_subject_type()->get_wealth_tribute_rate();
	} else if (commodity == defines::get()->get_regency_commodity()) {
		tribute_rate = this->get_game_data()->get_subject_type()->get_regency_tribute_rate();
	}

	if (tribute_rate == 0) {
		this->change_stored_commodity(commodity, tributable_quantity);
		return;
	}

	const int tribute = tributable_quantity * tribute_rate / 100;
	const int tributed_quantity = tributable_quantity - tribute;

	this->get_game_data()->get_overlord()->get_economy()->add_tributable_commodity(commodity, tribute, tribute_income_type);

	this->change_stored_commodity(commodity, tributed_quantity);

	if (tribute != 0 && commodity == defines::get()->get_wealth_commodity()) {
		this->get_game_data()->get_overlord()->get_turn_data()->add_income_transaction(tribute_income_type, tribute, nullptr, 0, this->domain);
		this->domain->get_turn_data()->add_expense_transaction(expense_transaction_type::tribute, tribute, nullptr, 0, this->get_game_data()->get_overlord());
	}
}

int64_t domain_economy::add_population_wealth(const int64_t wealth)
{
	int64_t weighted_population_size = 0;

	for (const auto &[population_type, population_type_size] : this->get_game_data()->get_population()->get_type_sizes()) {
		int64_t weighted_population_type_size = population_type_size;
		weighted_population_type_size *= get_population_strata_income_weight(population_type->get_strata());
		weighted_population_size += weighted_population_type_size;
	}

	if (weighted_population_size == 0) {
		return wealth;
	}
	
	int64_t remaining_wealth = wealth;

	for (population_unit *population_unit : this->get_game_data()->get_population_units()) {
		const population_strata population_unit_strata = population_unit->get_type()->get_strata();
		const int64_t population_unit_weighted_size = population_unit->get_size() * get_population_strata_income_weight(population_unit_strata);
		const int64_t population_unit_income = wealth * population_unit_weighted_size / weighted_population_size;
		const int64_t taxed_population_unit_income = population_unit_income * (100 - this->get_population_strata_tax_rate(population_unit_strata)) / 100;
		population_unit->change_wealth(taxed_population_unit_income);
		remaining_wealth -= taxed_population_unit_income;
	}

	assert_throw(remaining_wealth > 0);

	return remaining_wealth;
}

int64_t domain_economy::get_stored_food() const
{
	int64_t stored_food = 0;

	for (const auto &[commodity, quantity] : this->get_stored_commodities()) {
		if (commodity->is_food()) {
			stored_food += quantity;
		}
	}

	return stored_food;
}

void domain_economy::set_storage_capacity(const int64_t capacity)
{
	if (capacity == this->get_storage_capacity()) {
		return;
	}

	this->storage_capacity = capacity;

	if (game::get()->is_running()) {
		emit storage_capacity_changed();
	}
}

int64_t domain_economy::get_storage_capacity_for_commodity(const commodity *commodity) const
{
	int64_t storage_capacity = this->get_storage_capacity();

	if (commodity->get_units().empty()) {
		return storage_capacity;
	}

	const int64_t highest_unit_value = commodity->get_units().rbegin()->first;
	storage_capacity *= highest_unit_value;

	return storage_capacity;
}

QVariantList domain_economy::get_commodity_inputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_inputs());
}

int domain_economy::get_commodity_input(const QString &commodity_identifier) const
{
	return this->get_commodity_input(commodity::get(commodity_identifier.toStdString())).to_int();
}

void domain_economy::change_commodity_input(const commodity *commodity, const centesimal_int &change, const bool change_input_storage)
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

bool domain_economy::can_change_commodity_input(const commodity *commodity, const centesimal_int &change) const
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
		//for non-storable commodities, the commodity output is used directly instead of storage
		if (this->get_net_commodity_output(commodity) < change) {
			return false;
		}
	}

	return true;
}

QVariantList domain_economy::get_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_outputs());
}

qint64 domain_economy::get_commodity_output_int(const commodity *commodity) const
{
	return this->get_commodity_output(commodity).to_int64();
}

void domain_economy::change_commodity_output(const commodity *commodity, const centesimal_int &change)
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

void domain_economy::calculate_site_commodity_outputs()
{
	for (const province *province : this->get_game_data()->get_provinces()) {
		province->get_game_data()->calculate_site_commodity_outputs();
	}
}

void domain_economy::calculate_site_commodity_output(const commodity *commodity)
{
	for (const province *province : this->get_game_data()->get_provinces()) {
		province->get_game_data()->calculate_site_commodity_output(commodity);
	}
}

int domain_economy::get_food_output() const
{
	int food_output = 0;

	for (const auto &[commodity, output] : this->get_commodity_outputs()) {
		if (commodity->is_food()) {
			food_output += output.to_int();
		}
	}

	return food_output;
}

void domain_economy::change_commodity_demand(const commodity *commodity, const decimillesimal_int &change)
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

bool domain_economy::produces_commodity(const commodity *commodity) const
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

QVariantList domain_economy::get_bids_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_bids());
}

void domain_economy::set_bid(const commodity *commodity, const int value)
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

QVariantList domain_economy::get_offers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_offers());
}

void domain_economy::set_offer(const commodity *commodity, const int value)
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

void domain_economy::do_sale(const metternich::domain *other_domain, const commodity *commodity, const int sold_quantity, const bool state_purchase)
{
	this->change_stored_commodity(commodity, -sold_quantity);

	const int price = game::get()->get_price(commodity);
	const int sale_income = price * sold_quantity;
	this->add_tributable_commodity(defines::get()->get_wealth_commodity(), sale_income, income_transaction_type::tariff);
	this->domain->get_turn_data()->add_income_transaction(income_transaction_type::sale, sale_income, commodity, sold_quantity, other_domain != this->domain ? other_domain : nullptr);

	this->change_offer(commodity, -sold_quantity);

	domain_game_data *other_domain_game_data = other_domain->get_game_data();
	domain_economy *other_domain_economy = other_domain->get_economy();

	if (state_purchase) {
		other_domain_economy->change_stored_commodity(commodity, sold_quantity);
		const int purchase_expense = price * sold_quantity;
		other_domain_economy->change_wealth(-purchase_expense);
		other_domain->get_turn_data()->add_expense_transaction(expense_transaction_type::purchase, purchase_expense, commodity, sold_quantity, this->domain);

		other_domain_economy->change_bid(commodity, -sold_quantity);
	}

	//improve relations between the two countries after they traded (even if it was not a state purchase)
	if (this->domain != other_domain) {
		this->get_game_data()->change_base_opinion(other_domain, 1);
		other_domain_game_data->change_base_opinion(this->domain, 1);
	}
}

void domain_economy::calculate_commodity_needs()
{
	this->commodity_needs.clear();
}

void domain_economy::set_population_strata_tax_rate(const population_strata strata, const int value)
{
	if (value == this->get_population_strata_tax_rate(strata)) {
		return;
	}

	assert_throw(value >= 0);
	assert_throw(value <= 100);

	if (value == 0) {
		this->population_strata_tax_rates.erase(strata);
	} else {
		this->population_strata_tax_rates[strata] = value;
	}
}

void domain_economy::set_output_modifier(const centesimal_int &value)
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

void domain_economy::set_resource_output_modifier(const int value)
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

void domain_economy::set_industrial_output_modifier(const int value)
{
	if (value == this->get_industrial_output_modifier()) {
		return;
	}

	this->industrial_output_modifier = value;

	if (game::get()->is_running()) {
		emit industrial_output_modifier_changed();
	}
}

void domain_economy::set_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
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

void domain_economy::set_capital_commodity_output_modifier(const commodity *commodity, const centesimal_int &value)
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

void domain_economy::set_throughput_modifier(const int value)
{
	if (value == this->get_throughput_modifier()) {
		return;
	}

	this->throughput_modifier = value;

	if (game::get()->is_running()) {
		emit throughput_modifier_changed();
	}
}

void domain_economy::set_commodity_throughput_modifier(const commodity *commodity, const int value)
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

void domain_economy::change_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity, const int change)
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

void domain_economy::change_improvement_commodity_bonus(const improvement *improvement, const commodity *commodity, const centesimal_int &change)
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

void domain_economy::change_building_commodity_bonus(const building_type *building, const commodity *commodity, const int change)
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

void domain_economy::set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
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

void domain_economy::change_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change)
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

void domain_economy::change_settlement_commodity_bonus(const commodity *commodity, const centesimal_int &change)
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

void domain_economy::change_capital_commodity_bonus(const commodity *commodity, const centesimal_int &change)
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

void domain_economy::change_capital_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change)
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
