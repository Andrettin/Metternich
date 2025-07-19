#include "metternich.h"

#include "economy/employment_location.h"

#include "country/country.h"
#include "country/country_economy.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/profession.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_random_util.h"

namespace metternich {

const country *employment_location::get_employment_country() const
{
	return this->get_employment_site()->get_game_data()->get_owner();
}

const province *employment_location::get_employment_province() const
{
	return this->get_employment_site()->get_game_data()->get_province();
}

QVariantList employment_location::get_employees_qvariant_list() const
{
	return container::to_qvariant_list(this->get_employees());
}

bool employment_location::can_employ(const population_unit *population_unit, const population_type *&converted_population_type) const
{
	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	if (!profession->can_employ_with_conversion(population_unit->get_type(), converted_population_type)) {
		return false;
	}

	if (population_unit->get_employment_location() != this) {
		if (this->get_available_employment_capacity() == 0) {
			return false;
		}

		if (!this->can_fulfill_inputs_for_employment(population_unit)) {
			return false;
		}
	}

	return true;
}

bool employment_location::can_fulfill_inputs_for_employment(const population_unit *population_unit) const
{
	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	const commodity_map<int> inputs = this->get_employee_commodity_inputs(population_unit->get_type());

	if (inputs.empty() && profession->get_input_wealth() > 0) {
		return true;
	}

	if (this->get_employment_country() == nullptr) {
		return false;
	}

	const country_economy *country_economy = this->get_employment_country()->get_economy();

	for (const auto &[input_commodity, input_value] : inputs) {
		if (input_commodity->is_storable()) {
			if (country_economy->get_stored_commodity(input_commodity) < input_value) {
				return false;
			}
		} else {
			//for non-storable commodities, like Labor, the commodity output is used directly instead of storage
			if (country_economy->get_net_commodity_output(input_commodity) < input_value) {
				return false;
			}
		}
	}

	if (profession->get_input_wealth() != 0 && country_economy->get_wealth_with_credit() < country_economy->get_inflated_value(profession->get_input_wealth())) {
		return false;
	}

	return true;
}

void employment_location::add_employee(population_unit *employee)
{
	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);
	assert_throw(profession->can_employ(employee->get_type()));

	this->employees.push_back(employee);

	this->on_employee_added(employee, 1, true);

	assert_throw(this->get_available_employment_capacity() >= 0);
}


void employment_location::on_employee_added(population_unit *employee, const int multiplier, const bool change_input_storage)
{
	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	const commodity_map<centesimal_int> employee_outputs = this->get_employee_commodity_outputs(employee->get_type());
	for (const auto &[commodity, output] : employee_outputs) {
		this->change_total_employee_commodity_output(commodity, output * multiplier);
	}

	if (profession->get_output_commodity()->is_food() && this->is_resource_employment()) {
		//workers employed in resource food production do not need food themselves
		this->get_employment_province()->get_provincial_capital()->get_game_data()->change_free_food_consumption(1 * multiplier);
	}

	assert_throw(this->get_employment_country() != nullptr);
	country_economy *country_economy = this->get_employment_country()->get_economy();

	const commodity_map<int> inputs = this->get_employee_commodity_inputs(employee->get_type());
	for (const auto &[input_commodity, input_value] : inputs) {
		if (input_commodity->is_storable() && change_input_storage) {
			country_economy->change_stored_commodity(input_commodity, -input_value * multiplier);
		}
		country_economy->change_commodity_input(input_commodity, input_value * multiplier);
	}

	const int input_wealth = profession->get_input_wealth();
	//FIXME: apply production modifiers to input wealth

	if (input_wealth != 0) {
		if (change_input_storage) {
			country_economy->change_wealth(-input_wealth * multiplier);
		}
		country_economy->change_wealth_income(-input_wealth * multiplier);
	}
}

void employment_location::change_employment_capacity(const int change)
{
	if (change == 0) {
		return;
	}

	this->employment_capacity += change;

	if (this->get_available_employment_capacity() < 0) {
		this->check_excess_employment();
	}
}

commodity_map<int> employment_location::get_employee_commodity_inputs(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	if (profession->get_input_commodities().empty()) {
		return {};
	}

	commodity_map<decimillesimal_int> inputs;

	for (const auto &[commodity, input] : profession->get_input_commodities()) {
		inputs[commodity] += input;
	}

	const int output_modifier = population_type->get_profession_output_modifier(profession);

	for (auto &[commodity, input] : inputs) {
		input *= this->get_employment_output_multiplier();

		if (output_modifier != 0) {
			input *= 100 + output_modifier;
			input /= 100;
		}
	}

	commodity_map<int> ret_inputs;
	for (const auto &[commodity, input] : inputs) {
		assert_throw(input.get_fractional_value() == 0);
		ret_inputs[commodity] = input.to_int();
	}

	return ret_inputs;
}

commodity_map<centesimal_int> employment_location::get_employee_commodity_outputs(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	commodity_map<decimillesimal_int> outputs;

	const commodity *main_output_commodity = profession->get_output_commodity();
	outputs[main_output_commodity] = this->get_employee_main_commodity_output(population_type);

	const country *employment_country = this->get_employment_country();
	if (employment_country != nullptr) {
		const commodity_map<decimillesimal_int> &commodity_bonuses = employment_country->get_economy()->get_profession_commodity_bonuses(profession);
		for (auto &[commodity, bonus] : commodity_bonuses) {
			outputs[commodity] += bonus;
		}
	}

	const int output_modifier = population_type->get_profession_output_modifier(profession);
	if (output_modifier != 0) {
		for (auto &[commodity, output] : outputs) {
			output *= 100 + output_modifier;
			output /= 100;
		}
	}

	commodity_map<centesimal_int> ret_outputs;
	for (const auto &[commodity, output] : outputs) {
		ret_outputs[commodity] = centesimal_int(output);
	}

	return ret_outputs;
}

decimillesimal_int employment_location::get_employee_main_commodity_output(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	decimillesimal_int main_output(profession->get_output_value());
	main_output *= this->get_employment_output_multiplier();

	main_output += decimillesimal_int(population_type->get_profession_output_bonus(profession));

	if (this->is_resource_employment()) {
		main_output += population_type->get_resource_output_bonus();
	}

	return main_output;
}

void employment_location::change_total_employee_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int &count = (this->total_employee_commodity_outputs[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->total_employee_commodity_outputs.erase(commodity);
	}

	this->get_employment_site()->get_game_data()->change_base_commodity_output(commodity, change);
}

void employment_location::calculate_total_employee_commodity_outputs()
{
	commodity_map<centesimal_int> old_outputs = this->get_total_employee_commodity_outputs();

	commodity_map<centesimal_int> new_outputs;
	for (const auto &[commodity, output] : old_outputs) {
		new_outputs[commodity] = centesimal_int(0);
	}

	for (const population_unit *employee : this->get_employees()) {
		const commodity_map<centesimal_int> employee_outputs = this->get_employee_commodity_outputs(employee->get_type());

		for (const auto &[commodity, output] : employee_outputs) {
			new_outputs[commodity] += output;
		}
	}

	for (const auto &[commodity, output] : new_outputs) {
		const centesimal_int output_change = output - old_outputs[commodity];
		this->change_total_employee_commodity_output(commodity, output_change);
	}
}

void employment_location::check_excess_employment()
{
	//remove employees in excess of capacity
	while (this->get_available_employment_capacity() < 0) {
		this->decrease_employment(true, std::nullopt);
	}
}

void employment_location::check_superfluous_employment()
{
	if (this->get_employee_count() == 0) {
		return;
	}

	//remove employees who only produce fractional output, if together with others they don't contribute to integer output
	//for example, if the employment location has a single employee who produces 0.5 of a commodity, then this may result superfluous production: inputs will still be consumed, but 0 output will be available for transport (since transport uses integers)

	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	centesimal_int main_commodity_output = this->get_total_employee_commodity_outputs().find(profession->get_output_commodity())->second;

	bool changed = true;
	while (this->get_employee_count() > 0 && main_commodity_output.get_fractional_value() > 0 && changed) {
		changed = this->decrease_employment(true, centesimal_int::from_value(main_commodity_output.get_fractional_value()));

		if (changed && this->get_employee_count() > 0) {
			main_commodity_output = this->get_total_employee_commodity_outputs().find(profession->get_output_commodity())->second;
		}
	}
}

bool employment_location::decrease_employment(const bool change_input_storage, const std::optional<centesimal_int> &max_employee_output_value)
{
	assert_throw(this->get_employee_count() > 0);

	std::vector<population_unit *> potential_employees;

	centesimal_int lowest_output_value = centesimal_int::from_value(std::numeric_limits<int64_t>::max());
	if (max_employee_output_value.has_value()) {
		lowest_output_value = max_employee_output_value.value();
	}

	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	for (population_unit *employee : this->get_employees()) {
		const centesimal_int output_value = this->get_employee_commodity_outputs(employee->get_type())[profession->get_output_commodity()];

		if (output_value > lowest_output_value) {
			continue;
		} else if (output_value < lowest_output_value) {
			lowest_output_value = output_value;
			potential_employees.clear();
		}

		potential_employees.push_back(employee);
	}

	if (!max_employee_output_value.has_value()) {
		assert_throw(!potential_employees.empty());
	}

	if (potential_employees.empty()) {
		return false;
	}

	population_unit *employee_to_remove = vector::get_random(potential_employees);
	employee_to_remove->set_employment_location(nullptr, change_input_storage);

	return true;
}

}
