#include "metternich.h"

#include "infrastructure/wonder.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "script/condition/and_condition.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "technology/technology.h"

namespace metternich {

wonder::wonder(const std::string &identifier) : named_data_entry(identifier)
{
}

wonder::~wonder()
{
}
	
void wonder::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "cost_factor") {
		auto factor = std::make_unique<metternich::factor<country>>(100);
		database::process_gsml_data(factor, scope);
		this->cost_factor = std::move(factor);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "province_conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		database::process_gsml_data(conditions, scope);
		this->province_conditions = std::move(conditions);
	} else if (tag == "province_modifier") {
		this->province_modifier = std::make_unique<modifier<const province>>();
		database::process_gsml_data(this->province_modifier, scope);
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void wonder::initialize()
{
	this->get_building()->get_building_class()->get_slot_type()->add_wonder(this);

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_wonder(this);
	}

	if (this->obsolescence_technology != nullptr) {
		this->obsolescence_technology->add_disabled_wonder(this);
	}

	named_data_entry::initialize();
}

void wonder::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Wonder \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_building() == nullptr) {
		throw std::runtime_error(std::format("Wonder \"{}\" has no building type.", this->get_identifier()));
	}

	if (!this->get_building()->is_provincial()) {
		throw std::runtime_error(std::format("Wonder \"{}\" has a non-provincial building type.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_province_conditions() != nullptr) {
		this->get_province_conditions()->check_validity();
	}
}

int wonder::get_wealth_cost_for_country(const country *country) const
{
	int cost = this->get_wealth_cost();

	if (cost > 0 && this->get_cost_factor() != nullptr) {
		cost = this->get_cost_factor()->calculate(country, centesimal_int(cost)).to_int();
		cost = std::max(1, cost);
	}

	return cost;
}

commodity_map<int> wonder::get_commodity_costs_for_country(const country *country) const
{
	commodity_map<int> costs = this->get_commodity_costs();

	for (auto &[commodity, cost] : costs) {
		if (cost > 0) {
			if (country->get_game_data()->get_wonder_cost_efficiency_modifier() != 0) {
				cost *= 100;
				cost /= 100 + country->get_game_data()->get_wonder_cost_efficiency_modifier();
			}

			if (this->get_cost_factor() != nullptr) {
				cost = this->get_cost_factor()->calculate(country, centesimal_int(cost)).to_int();
			}

			cost = std::max(1, cost);
		}
	}

	return costs;
}

}
