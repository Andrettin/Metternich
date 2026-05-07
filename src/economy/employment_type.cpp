#include "metternich.h"

#include "economy/employment_type.h"

#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "economy/commodity.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population_type.h"
#include "script/modifier.h"
#include "util/assert_util.h"

namespace metternich {
	
employment_type::employment_type(const std::string &identifier) : named_data_entry(identifier)
{
}

employment_type::~employment_type()
{
}

void employment_type::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "monthly_output_value") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		assert_throw(this->get_output_commodity() != nullptr);
		this->monthly_output_value = this->get_output_commodity()->string_to_value(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void employment_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "input_commodities") {
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->input_commodities[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "employee_types") {
		for (const std::string &value : values) {
			this->employee_types.insert(population_type::get(value));
		}
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		this->modifier->process_gsml_data(scope);
	} else if (tag == "domain_modifier") {
		this->domain_modifier = std::make_unique<metternich::modifier<const domain>>();
		this->domain_modifier->process_gsml_data(scope);
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void employment_type::check() const
{
	if (this->get_employee_types().empty()) {
		throw std::runtime_error(std::format("Employment type \"{}\" has no employee types.", this->get_identifier()));
	}

	if (this->get_output_commodity() != nullptr && this->get_base_employment_size() == 0) {
		throw std::runtime_error(std::format("Employment type \"{}\" has an output commodity, but no base employment size.", this->get_identifier()));
	}
}

bool employment_type::can_employ(const population_type *population_type, const metternich::population_type *&employed_population_type) const
{
	employed_population_type = nullptr;

	if (this->get_employee_types().contains(population_type)) {
		employed_population_type = population_type;
		return true;
	}

	for (const metternich::population_type *equivalent_population_type : population_type->get_equivalent_population_types()) {
		if (this->get_employee_types().contains(equivalent_population_type)) {
			employed_population_type = equivalent_population_type;
			return true;
		}
	}

	return false;
}

int64_t employment_type::get_input_for_employment_size(const commodity *commodity, const int64_t employment_size) const
{
	assert_throw(this->get_input_commodities().contains(commodity));

	int64_t input = this->get_input_commodities().find(commodity)->second;

	input *= employment_size;
	input /= this->get_base_employment_size();

	input *= game::get()->get_current_months_per_turn();

	return std::max(input, 1ll);
}

int64_t employment_type::get_employment_size_for_input(const commodity *commodity, const int64_t input) const
{
	assert_throw(this->get_input_commodities().contains(commodity));

	int64_t employment_size = input;
	employment_size /= game::get()->get_current_months_per_turn();

	employment_size *= this->get_base_employment_size();
	employment_size /= this->get_input_commodities().find(commodity)->second;

	return employment_size;
}

bool employment_type::is_available_for_site(const site *site) const
{
	std::vector<const commodity *> commodities;
	if (this->get_output_commodity() != nullptr) {
		commodities.push_back(this->get_output_commodity());
	}
	for (const auto &[input_commodity, input_value] : this->get_input_commodities()) {
		commodities.push_back(input_commodity);
	}

	for (const commodity *commodity : commodities) {
		if (site->get_game_data()->get_owner() == nullptr) {
			return false;
		}

		if (!site->get_game_data()->get_owner()->get_economy()->get_available_commodities().contains(commodity)) {
			return false;
		}

		if (commodity->get_required_technology() != nullptr) {
			if (!site->get_game_data()->get_province()->get_game_data()->has_technology(commodity->get_required_technology())) {
				return false;
			}
		}
	}

	return true;
}

}
