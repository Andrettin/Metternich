#include "metternich.h"

#include "population/population_unit.h"

#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "economy/employment_type.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population.h"
#include "population/population_defines.h"
#include "population/population_strata.h"
#include "population/population_type.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/factor.h"
#include "script/mean_time_to_happen.h"
#include "species/phenotype.h"
#include "species/species.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

population_unit::population_unit()
{
	connect(this, &population_unit::type_changed, this, &population_unit::icon_changed);
}

population_unit::population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::employment_type *employment_type, const int64_t size, const decimillesimal_int &literacy_rate, const metternich::site *site) : population_unit()
{
	this->type = type;
	this->culture = culture;
	this->religion = religion;
	this->phenotype = phenotype;
	this->employment_type = employment_type;
	this->size = size;
	this->literacy_rate = literacy_rate;
	this->site = site;

	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_size() > 0);
	assert_throw(this->get_literacy_rate() >= 0);

	if (!vector::contains(this->get_culture()->get_species(), this->get_species())) {
		throw std::runtime_error(std::format("Tried to create a population unit with a species (\"{}\") which is not allowed for its culture (\"{}\").", this->get_species()->get_identifier(), this->get_culture()->get_identifier()));
	}

	this->set_domain(site->get_game_data()->get_owner());
	assert_throw(this->get_domain() != nullptr);
}

population_unit::population_unit(const gsml_data &scope) : population_unit()
{
	scope.process(this);

	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_domain() != nullptr);
	assert_throw(this->get_site() != nullptr);
}

void population_unit::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type") {
		this->type = population_type::get(value);
	} else if (key == "culture") {
		this->culture = culture::get(value);
	} else if (key == "religion") {
		this->religion = religion::get(value);
	} else if (key == "phenotype") {
		this->phenotype = phenotype::get(value);
	} else if (key == "employment_type") {
		this->employment_type = employment_type::get(value);
	} else if (key == "domain") {
		this->set_domain(domain::get(value));
	} else if (key == "site") {
		this->site = site::get(value);
	} else if (key == "size") {
		this->size = std::stoll(value);
	} else if (key == "literacy_rate") {
		this->literacy_rate = decimillesimal_int(value);
	} else if (key == "wealth") {
		this->wealth = decimillesimal_int(value);
	} else if (key == "sick_percent") {
		this->sick_percent = decimillesimal_int(value);
	} else if (key == "starving_percent") {
		this->starving_percent = decimillesimal_int(value);
	} else if (key == "fulfilled_everyday_needs_percent") {
		this->fulfilled_everyday_needs_percent = std::stoi(value);
	} else if (key == "fulfilled_luxury_needs_percent") {
		this->fulfilled_luxury_needs_percent = std::stoi(value);
	} else {
		throw std::runtime_error(std::format("Invalid population unit property: \"{}\".", key));
	}
}

void population_unit::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	throw std::runtime_error(std::format("Invalid population unit scope: \"{}\".", tag));
}

gsml_data population_unit::to_gsml_data() const
{
	gsml_data data;

	data.add_property("type", this->type->get_identifier());
	data.add_property("culture", this->culture->get_identifier());
	data.add_property("religion", this->religion->get_identifier());
	data.add_property("phenotype", this->phenotype->get_identifier());
	data.add_property("domain", this->domain->get_identifier());
	data.add_property("site", this->site->get_identifier());
	data.add_property("size", std::to_string(this->size));
	data.add_property("literacy_rate", this->literacy_rate.to_string());
	data.add_property("wealth", this->wealth.to_string());
	data.add_property("sick_percent", this->sick_percent.to_string());
	data.add_property("starving_percent", this->starving_percent.to_string());
	data.add_property("fulfilled_everyday_needs_percent", std::to_string(this->fulfilled_everyday_needs_percent));
	data.add_property("fulfilled_luxury_needs_percent", std::to_string(this->fulfilled_luxury_needs_percent));

	if (this->employment_type != nullptr) {
		data.add_property("employment_type", this->employment_type->get_identifier());
	}

	return data;
}

QCoro::Task<void> population_unit::do_cultural_change()
{
	assert_throw(this->get_site()->get_game_data()->get_population()->get_size() > 0);
	assert_throw(this->get_province()->get_game_data()->get_population()->get_size() > 0);
	assert_throw(this->get_domain()->get_game_data()->get_population()->get_size() > 0);

	for (const std::unique_ptr<const culture::cultural_derivation> &cultural_derivation : this->get_culture()->get_cultural_derivations()) {
		decimillesimal_int derivation_rate(0);

		if (cultural_derivation->conditions != nullptr && !cultural_derivation->conditions->check(this)) {
			continue;
		}

		if (cultural_derivation->mean_time_to_happen != nullptr) {
			const decimillesimal_int mtth = cultural_derivation->mean_time_to_happen->calculate(this, game::get()->get_year());
			derivation_rate = decimillesimal_int(100) / mtth;
		}

		if (derivation_rate == 0) {
			continue;
		}

		const int64_t derived_size = std::max((this->get_size() * derivation_rate / 100).to_ceil_int64(), 1ll);
		const decimillesimal_int lost_wealth = this->get_wealth() * derived_size / this->get_size();

		const population_type *culture_population_type = cultural_derivation->culture->get_population_class_type(this->get_type()->get_population_class());

		if (!this->get_site()->get_game_data()->can_have_population_type(culture_population_type)) {
			continue;
		}

		co_await this->get_site()->get_game_data()->change_population(culture_population_type, cultural_derivation->culture, this->get_religion(), this->get_phenotype(), nullptr, derived_size, this->get_literacy_rate(), lost_wealth, true);

		co_await this->get_site()->get_game_data()->change_population(this->get_type(), this->get_culture(), this->get_religion(), this->get_phenotype(), this->get_employment_type(), -derived_size, this->get_literacy_rate(), -lost_wealth, true);
	}
}

QCoro::Task<void> population_unit::do_promotion()
{
	co_await this->do_promotion(false);
	co_await this->do_promotion(true);
}

QCoro::Task<void> population_unit::do_promotion(const bool is_demotion)
{
	assert_throw(this->get_site()->get_game_data()->get_population()->get_size() > 0);
	assert_throw(this->get_province()->get_game_data()->get_population()->get_size() > 0);
	assert_throw(this->get_domain()->get_game_data()->get_population()->get_size() > 0);

	decimillesimal_int promotion_rate = population_defines::get()->get_base_monthly_promotion_rate() * game::get()->get_current_months_per_turn();

	if (is_demotion) {
		const factor<population_unit> *demotion_chance = population_defines::get()->get_demotion_chance();
		const decimillesimal_int demotion_chance_result = demotion_chance->calculate(this);
		promotion_rate *= demotion_chance_result;
	} else {
		const factor<population_unit> *promotion_chance = population_defines::get()->get_promotion_chance();
		const decimillesimal_int promotion_chance_result = promotion_chance->calculate(this);
		promotion_rate *= promotion_chance_result;
	}

	if (promotion_rate == 0) {
		co_return;
	}

	const population_type *best_promotion_type = nullptr;
	decimillesimal_int best_promotion_factor_result;

	for (const auto &[promotion_type, promotion_factor] : this->get_type()->get_promotion_factors()) {
		if (is_demotion) {
			//when demoting, population units can only demote to population types in the same strata or lower
			if (promotion_type->get_strata() > this->get_type()->get_strata()) {
				continue;
			}
		} else {
			//when promoting, population units can only promote to population types in the same strata or higher
			if (promotion_type->get_strata() < this->get_type()->get_strata()) {
				continue;
			}
		}

		if (!this->get_site()->get_game_data()->can_have_population_type(promotion_type)) {
			continue;
		}

		const decimillesimal_int promotion_factor_result = promotion_factor->calculate(this);
		if (promotion_factor_result > 0 && promotion_factor_result > best_promotion_factor_result) {
			best_promotion_factor_result = promotion_factor_result;
			best_promotion_type = promotion_type;
		}
	}

	if (best_promotion_type == nullptr) {
		co_return;
	}

	const int64_t promoted_size = (this->get_size() * promotion_rate / 100).to_int64();
	const decimillesimal_int lost_wealth = this->get_wealth() * promoted_size / this->get_size();

	co_await this->get_site()->get_game_data()->change_population(best_promotion_type, this->get_culture(), this->get_religion(), this->get_phenotype(), nullptr, promoted_size, this->get_literacy_rate(), lost_wealth, true);

	co_await this->get_site()->get_game_data()->change_population(this->get_type(), this->get_culture(), this->get_religion(), this->get_phenotype(), this->get_employment_type(), -promoted_size, this->get_literacy_rate(), -lost_wealth, true);
}

std::string population_unit::get_scope_name() const
{
	return std::format("{} {}", this->get_culture()->get_name(), this->get_type()->get_name());
}

const icon *population_unit::get_icon() const
{
	return this->get_type()->get_phenotype_icon(this->get_phenotype());
}

void population_unit::set_type(const population_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	assert_throw(type != nullptr);

	this->get_site()->get_game_data()->get_population()->change_type_size(this->get_type(), -this->get_size());

	this->type = type;

	this->get_site()->get_game_data()->get_population()->change_type_size(this->get_type(), this->get_size());

	emit type_changed();
}

void population_unit::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	assert_throw(culture != nullptr);
	assert_throw(vector::contains(culture->get_species(), this->get_species()));

	this->get_site()->get_game_data()->get_population()->change_culture_size(this->get_culture(), -this->get_size());

	this->culture = culture;

	this->get_site()->get_game_data()->get_population()->change_culture_size(this->get_culture(), this->get_size());

	const population_type *culture_population_type = culture->get_population_class_type(this->get_type()->get_population_class());
	if (culture_population_type != this->get_type()) {
		this->set_type(culture_population_type);
	}

	emit culture_changed();
}

void population_unit::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->get_site()->get_game_data()->get_population()->change_religion_size(this->get_religion(), -this->get_size());

	this->religion = religion;

	this->get_site()->get_game_data()->get_population()->change_religion_size(this->get_religion(), this->get_size());

	emit religion_changed();
}

void population_unit::set_phenotype(const metternich::phenotype *phenotype)
{
	if (phenotype == this->get_phenotype()) {
		return;
	}

	assert_throw(phenotype != nullptr);
	assert_throw(vector::contains(this->get_culture()->get_species(), phenotype->get_species()));

	this->get_site()->get_game_data()->get_population()->change_phenotype_size(this->get_phenotype(), -this->get_size());

	this->phenotype = phenotype;

	this->get_site()->get_game_data()->get_population()->change_phenotype_size(this->get_phenotype(), this->get_size());

	emit phenotype_changed();
}

const species *population_unit::get_species() const
{
	return this->get_phenotype()->get_species();
}

QCoro::Task<void> population_unit::set_employment_type(const metternich::employment_type *employment_type)
{
	if (employment_type == this->get_employment_type()) {
		co_return;
	}

	if (this->get_employment_type() != nullptr) {
		co_await this->get_site()->get_game_data()->change_employment_size(this->get_employment_type(), -this->get_size(), true);
	}

	this->employment_type = employment_type;

	if (this->get_employment_type() != nullptr) {
		co_await this->get_site()->get_game_data()->change_employment_size(this->get_employment_type(), this->get_size(), true);
	}

	emit employment_type_changed();
}

const icon *population_unit::get_small_icon() const
{
	return this->get_type()->get_phenotype_small_icon(this->get_phenotype());
}

void population_unit::set_domain(const metternich::domain *domain)
{
	if (domain == this->get_domain()) {
		return;
	}

	if (this->get_domain() != nullptr) {
		this->get_domain()->get_game_data()->remove_population_unit(this);
	}

	this->domain = domain;

	if (this->get_domain() != nullptr) {
		this->get_domain()->get_game_data()->add_population_unit(this);
	}

	emit domain_changed();
}

const province *population_unit::get_province() const
{
	if (this->get_site() == nullptr) {
		return nullptr;
	}

	return this->get_site()->get_game_data()->get_province();
}

void population_unit::set_site(const metternich::site *site)
{
	if (site == this->get_site()) {
		return;
	}

	const province *old_province = this->get_province();

	this->site = site;

	emit site_changed();

	const province *province = this->get_province();

	if (old_province != province) {
		emit province_changed();
	}

	const metternich::domain *domain = site ? site->get_game_data()->get_owner() : nullptr;
	this->set_domain(domain);
}

[[nodiscard]] QCoro::Task<void> population_unit::set_size(const int64_t size, const bool change_input_storage)
{
	if (size == this->get_size()) {
		co_return;
	}

	assert_throw(size >= 0);
	assert_throw(this->get_site() != nullptr);

	if (this->get_employment_type() != nullptr) {
		co_await this->get_site()->get_game_data()->change_employment_size(this->get_employment_type(), -this->get_size(), change_input_storage);
	}

	this->get_site()->get_game_data()->get_population()->on_population_unit_lost(this);

	this->size = size;

	this->get_site()->get_game_data()->get_population()->on_population_unit_gained(this);

	if (this->get_employment_type() != nullptr) {
		co_await this->get_site()->get_game_data()->change_employment_size(this->get_employment_type(), this->get_size(), change_input_storage);
	}

	emit size_changed();
}

int64_t population_unit::get_weighted_size() const
{
	const population_strata strata = this->get_type()->get_strata();
	const int64_t weighted_size = this->get_size() * get_population_strata_income_weight(strata);
	return weighted_size;
}

int64_t population_unit::get_literate_size() const
{
	return (this->get_size() * this->get_literacy_rate() / 100).to_int64();
}

void population_unit::set_literacy_rate(const decimillesimal_int &literacy_rate)
{
	if (literacy_rate == this->get_literacy_rate()) {
		return;
	}

	if (literacy_rate > 100) {
		this->set_literacy_rate(decimillesimal_int(100));
		return;
	}

	if (literacy_rate < 0) {
		this->set_literacy_rate(decimillesimal_int(0));
		return;
	}

	this->get_site()->get_game_data()->get_population()->change_literate_size(-this->get_literate_size());

	this->literacy_rate = literacy_rate;

	this->get_site()->get_game_data()->get_population()->change_literate_size(this->get_literate_size());

	emit literacy_rate_changed();
}

QString population_unit::get_wealth_qstring() const
{
	return defines::get()->get_wealth_commodity()->value_to_qstring(this->get_wealth());
}

void population_unit::set_wealth(const decimillesimal_int &wealth)
{
	if (wealth == this->get_wealth()) {
		return;
	}

	assert_throw(wealth >= 0);

	this->wealth = wealth;

	emit wealth_changed();
}

decimillesimal_int population_unit::get_population_growth_rate() const
{
	assert_throw(this->get_site() != nullptr);

	decimillesimal_int population_growth_rate = this->get_site()->get_game_data()->get_population_growth_rate();

	if (this->get_starving_percent() > 0) {
		if (population_growth_rate > 0) {
			//starvation adds to a negative population growth rate, but replaces a positive growth rate (in proportion to the starving percent)
			population_growth_rate = population_growth_rate * (100 - this->get_starving_percent()) / 100;
		}

		//starvation affects the unit's population growth rate
		population_growth_rate += population_defines::get()->get_quarterly_starvation_mortality_rate() * game::get()->get_current_quarters_per_turn() * this->get_starving_percent() / 100 * -1;
	}

	return population_growth_rate;
}

void population_unit::set_sick_percent(const decimillesimal_int &percent)
{
	if (percent == this->get_sick_percent()) {
		return;
	}

	this->sick_percent = percent;

	emit sick_percent_changed();
}

void population_unit::set_starving_percent(const decimillesimal_int &percent)
{
	if (percent == this->get_starving_percent()) {
		return;
	}

	this->starving_percent = percent;

	emit starving_percent_changed();

	if (this->get_site() != nullptr) {
		emit this->get_site()->get_game_data()->population_growth_rate_changed();
	}
}

bool population_unit::is_food_producer() const
{
	if (this->get_type()->get_output_commodity() != nullptr && this->get_type()->get_output_commodity()->is_food()) {
		return true;
	}

	if (this->get_employment_type() != nullptr && this->get_employment_type()->get_output_commodity() != nullptr && this->get_employment_type()->get_output_commodity()->is_food()) {
		return true;
	}

	return false;
}

std::map<food_type, decimillesimal_int> population_unit::get_food_consumption(decimillesimal_int &total_food_consumption) const
{
	total_food_consumption = decimillesimal_int(0);

	if (this->is_food_producer()) {
		//food producers don't need to consume food
		return {};
	}

	std::map<food_type, decimillesimal_int> food_consumption = population_defines::get()->get_population_food_consumption();

	for (auto &[food_type, consumption] : food_consumption) {
		//adjust food consumption according to this population unit's size
		consumption *= this->get_size();
		consumption /= population_defines::get()->get_base_population_size();

		total_food_consumption += consumption;
	}

	return food_consumption;
}

std::map<food_type, decimillesimal_int> population_unit::get_food_consumption_after_free_consumption(decimillesimal_int &total_food_consumption, decimillesimal_int *remaining_free_food_consumption) const
{
	std::map<food_type, decimillesimal_int> food_consumption = this->get_food_consumption(total_food_consumption);

	decimillesimal_int total_free_consumption;

	if (remaining_free_food_consumption != nullptr) {
		if ((*remaining_free_food_consumption) > 0) {
			total_free_consumption = decimillesimal_int::min(total_food_consumption, *remaining_free_food_consumption);
			*remaining_free_food_consumption -= total_free_consumption;
		}
	}

	if (total_free_consumption == total_food_consumption) {
		//if the total free food consumption for this population unit equals the total food consumption for it, then we don't need to go through the food types, we can just consider the population unit fed
		//this also prevents rounding errors when calculating the free consumption for each food type leaving a tiny amount of starvation even if the population unit should be completely fed
		return {};
	}

	for (auto &[food_type, consumption] : food_consumption) {
		if (consumption == 0) {
			continue;
		}

		if (total_free_consumption > 0) {
			const decimillesimal_int free_consumption = total_free_consumption * consumption / total_food_consumption;
			if (free_consumption > 0) {
				consumption -= free_consumption;
			}
		}
	}

	return food_consumption;
}

void population_unit::consume_food_commodity(const commodity *food_commodity, decimillesimal_int &consumption)
{
	const decimillesimal_int &stored_commodity = this->get_domain()->get_economy()->get_stored_commodity(food_commodity);
	if (stored_commodity == 0) {
		return;
	}

	const decimillesimal_int consumed_quantity = decimillesimal_int::min(stored_commodity, consumption);
	this->get_domain()->get_economy()->change_stored_commodity(food_commodity, -consumed_quantity);

	consumption -= consumed_quantity;
}

void population_unit::consume_imported_food_commodity(const commodity *food_commodity, decimillesimal_int &consumption, const std::vector<const metternich::domain *> &trade_domains)
{
	if (!this->domain->get_economy()->can_trade_commodity(food_commodity)) {
		return;
	}

	for (const metternich::domain *trade_domain : trade_domains) {
		if (trade_domain == this->get_domain()) {
			continue;
		}

		const decimillesimal_int domain_offer = trade_domain->get_economy()->get_offer(food_commodity);
		if (domain_offer == 0) {
			continue;
		}

		decimillesimal_int bought_quantity = decimillesimal_int::min(domain_offer, consumption);

		//ensure that we have enough storage for the purchase
		bought_quantity = decimillesimal_int::min(bought_quantity, this->get_domain()->get_economy()->get_storage_capacity_for_commodity(food_commodity));

		//ensure that we can afford to purchase the food commodity
		bought_quantity = decimillesimal_int::min(bought_quantity, this->get_domain()->get_economy()->get_wealth() / game::get()->get_price(food_commodity));

		if (bought_quantity <= 0) {
			//cannot afford to buy the commodity, or does not have storage capacity for it, so there's no point in doing the same process for other domains
			break;
		}

		trade_domain->get_economy()->do_sale(this->get_domain(), food_commodity, bought_quantity, true, false);

		this->get_domain()->get_economy()->change_stored_commodity(food_commodity, -bought_quantity);
		consumption -= bought_quantity;

		if (consumption == 0) {
			break;
		}
	}
}

void population_unit::consume_food(const std::vector<const metternich::domain *> &trade_domains, const std::map<food_type, std::vector<const commodity *>> &food_type_commodities, const std::vector<const commodity *> &food_commodities, std::unordered_map<const metternich::site *, decimillesimal_int> &remaining_free_food_consumption_per_holding)
{
	this->set_sick_percent(decimillesimal_int(0));
	this->set_starving_percent(decimillesimal_int(0));

	decimillesimal_int *remaining_holding_free_consumption = nullptr;

	auto free_consumption_find_iterator = remaining_free_food_consumption_per_holding.find(this->get_site());
	if (free_consumption_find_iterator != remaining_free_food_consumption_per_holding.end()) {
		remaining_holding_free_consumption = &free_consumption_find_iterator->second;
	}

	decimillesimal_int total_food_consumption;
	std::map<food_type, decimillesimal_int> food_consumption = this->get_food_consumption_after_free_consumption(total_food_consumption, remaining_holding_free_consumption);

	decimillesimal_int total_wrong_food_consumption;
	decimillesimal_int total_remaining_food_consumption;

	if (total_food_consumption == 0) {
		return;
	}

	for (auto &[food_type, consumption] : food_consumption) {
		if (consumption == 0) {
			continue;
		}

		const auto commodities_find_iterator = food_type_commodities.find(food_type);
		assert_throw(commodities_find_iterator != food_type_commodities.end());

		const std::vector<const commodity *> &food_type_commodity_vector = commodities_find_iterator->second;

		//do food consumption
		for (const commodity *food_commodity : food_type_commodity_vector) {
			this->consume_food_commodity(food_commodity, consumption);

			if (consumption == 0) {
				break;
			}
		}

		if (consumption == 0) {
			continue;
		}

		//import remaining consumption for this food type
		for (const commodity *food_commodity : food_type_commodity_vector) {
			this->consume_imported_food_commodity(food_commodity, consumption, trade_domains);

			if (consumption == 0) {
				break;
			}
		}

		if (consumption == 0) {
			continue;
		}

		total_wrong_food_consumption = consumption;

		//try to consume food of the wrong type if possible (better than suffering starvation)
		for (const commodity *food_commodity : food_commodities) {
			if (food_commodity->get_food_type() == food_type) {
				//already checked
				continue;
			}

			this->consume_food_commodity(food_commodity, consumption);

			if (consumption == 0) {
				break;
			}
		}

		if (consumption == 0) {
			continue;
		}

		//try to import food of the wrong type if possible
		for (const commodity *food_commodity : food_commodities) {
			if (food_commodity->get_food_type() == food_type) {
				//already checked
				continue;
			}

			this->consume_imported_food_commodity(food_commodity, consumption, trade_domains);

			if (consumption == 0) {
				break;
			}
		}

		total_wrong_food_consumption -= consumption;
		total_remaining_food_consumption += consumption;
	}

	//set how many people in this population unit are sick because of not having a balanced enough diet
	if (total_wrong_food_consumption > 0) {
		this->set_sick_percent(100 * total_wrong_food_consumption / total_food_consumption);
	}

	//set how many people in this population unit are starving if not all food consumption could be fulfilled
	if (total_remaining_food_consumption > 0) {
		this->set_starving_percent(100 * total_remaining_food_consumption / total_food_consumption);
	}
}

void population_unit::purchase_needs(const decimillesimal_int &consumption_wealth, const std::vector<const metternich::domain *> &trade_domains, commodity_map<decimillesimal_int> &remaining_demands)
{
	const int total_need_weight = this->get_type()->get_total_everyday_need_weight() + this->get_type()->get_total_luxury_need_weight();

	if (total_need_weight == 0) {
		return;
	}

	this->set_fulfilled_everyday_needs_percent(this->purchase_needs(consumption_wealth * this->get_type()->get_total_everyday_need_weight() / total_need_weight, this->get_type()->get_everyday_need_weights(), trade_domains, remaining_demands));
	this->set_fulfilled_luxury_needs_percent(this->purchase_needs(consumption_wealth * this->get_type()->get_total_luxury_need_weight() / total_need_weight, this->get_type()->get_luxury_need_weights(), trade_domains, remaining_demands));
}

int population_unit::purchase_needs(const decimillesimal_int &consumption_wealth, const commodity_map<int> &need_weights, const std::vector<const metternich::domain *> &trade_domains, commodity_map<decimillesimal_int> &remaining_demands)
{
	int fulfilled_percent = 0;
	int total_need_weight = 0;

	for (const auto &[commodity, need_weight] : need_weights) {
		if (!this->domain->get_economy()->can_trade_commodity(commodity)) {
			continue;
		}

		total_need_weight += need_weight;
	}

	if (total_need_weight == 0) {
		return 100;
	}

	for (const auto &[commodity, need_weight] : need_weights) {
		if (!this->domain->get_economy()->can_trade_commodity(commodity)) {
			continue;
		}

		const decimillesimal_int commodity_wealth_need = consumption_wealth * need_weight / total_need_weight;

		if (commodity_wealth_need == 0) {
			fulfilled_percent += 100 * need_weight;
			continue;
		}

		const decimillesimal_int commodity_need = commodity->wealth_value_to_value(commodity_wealth_need);

		if (commodity_need == 0) {
			fulfilled_percent += 100 * need_weight;
			continue;
		}

		decimillesimal_int affordable_commodity_need = decimillesimal_int::min(commodity_need, this->get_wealth() / game::get()->get_price(commodity));

		if (affordable_commodity_need == 0) {
			continue;
		}

		decimillesimal_int fulfilled_commodity_need;

		for (const metternich::domain *trade_domain : trade_domains) {
			const decimillesimal_int domain_offer = trade_domain->get_economy()->get_offer(commodity);
			if (domain_offer == 0) {
				continue;
			}

			assert_throw(domain_offer > 0);
			assert_throw(affordable_commodity_need > 0);

			const decimillesimal_int bought_quantity = decimillesimal_int::min(domain_offer, affordable_commodity_need);
			assert_throw(bought_quantity > 0);

			trade_domain->get_economy()->do_sale(this->domain, commodity, bought_quantity, false, false);

			const decimillesimal_int purchase_price = bought_quantity * game::get()->get_price(commodity);
			this->change_wealth(-purchase_price);

			affordable_commodity_need -= bought_quantity;
			fulfilled_commodity_need += bought_quantity;
			if (affordable_commodity_need == 0) {
				break;
			}
		}

		fulfilled_percent += (fulfilled_commodity_need * 100 / commodity_need).to_int() * need_weight;

		if (fulfilled_commodity_need < affordable_commodity_need) {
			remaining_demands[commodity] += affordable_commodity_need - fulfilled_commodity_need;
		}
	}

	fulfilled_percent /= total_need_weight;
	return fulfilled_percent;
}

}

#include "moc_population_unit.cpp"
