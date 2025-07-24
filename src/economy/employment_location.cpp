#include "metternich.h"

#include "economy/employment_location.h"

#include "country/country.h"
#include "country/country_economy.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/profession.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"
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

profession_map<std::vector<population_unit *>> employment_location::take_employees()
{
	profession_map<std::vector<population_unit *>> employees_by_profession;

	const std::vector<population_unit *> employees = this->get_employees();
	for (population_unit *employee : employees) {
		employees_by_profession[employee->get_profession()].push_back(employee);
		employee->set_employment_location(nullptr, nullptr, true);
	}

	return employees_by_profession;
}

bool employment_location::can_employ(const population_unit *population_unit, const profession *profession, const population_type * &converted_population_type) const
{
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));

	if (!profession->can_employ_with_conversion(population_unit->get_type(), converted_population_type)) {
		return false;
	}

	if (profession->get_required_technology() != nullptr) {
		const country *employment_country = this->get_employment_country();
		if (employment_country == nullptr || !employment_country->get_game_data()->has_technology(profession->get_required_technology())) {
			return false;
		}
	}

	if (population_unit->get_employment_location() != this) {
		if (this->get_available_production_capacity() == 0) {
			return false;
		}

		if (!this->can_fulfill_inputs_for_employment(population_unit, profession)) {
			return false;
		}
	}

	return true;
}

bool employment_location::can_fulfill_inputs_for_employment(const population_unit *population_unit, const profession *profession) const
{
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));

	const commodity_map<centesimal_int> inputs = this->get_employee_commodity_inputs(population_unit->get_type(), profession);

	if (inputs.empty() && profession->get_input_wealth() > 0) {
		return true;
	}

	if (this->get_employment_country() == nullptr) {
		return false;
	}

	const country_economy *country_economy = this->get_employment_country()->get_economy();

	for (const auto &[input_commodity, input_value] : inputs) {
		if (!country_economy->can_change_commodity_input(input_commodity, input_value)) {
			return false;
		}
	}

	if (profession->get_input_wealth() != 0 && country_economy->get_wealth_with_credit() < country_economy->get_inflated_value(profession->get_input_wealth())) {
		return false;
	}

	return true;
}

void employment_location::add_employee(population_unit *employee, const profession *profession)
{
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));
	assert_throw(profession->can_employ(employee->get_type()));

	this->employees.push_back(employee);

	this->on_employee_added(employee, profession, 1, true);

	assert_throw(this->get_available_production_capacity() >= 0);
}

void employment_location::add_employees_if_possible(const profession_map<std::vector<population_unit *>> &employees_by_profession)
{
	for (const auto &[profession, employees] : employees_by_profession) {
		for (population_unit *employee : employees) {
			const population_type *converted_population_type = nullptr;
			if (this->can_employ(employee, profession, converted_population_type)) {
				employee->set_employment_location(this, profession, true);
			}
		}
	}
}

void employment_location::on_employee_added(population_unit *employee, const profession *profession, const int multiplier, const bool change_input_storage)
{
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));

	const centesimal_int employee_production_capacity = profession->get_output_value() * (100 + employee->get_type()->get_profession_output_modifier(profession)) / 100;
	this->change_employed_production_capacity(employee_production_capacity * multiplier);

	const commodity_map<centesimal_int> employee_outputs = this->get_employee_commodity_outputs(employee->get_type(), profession);
	for (const auto &[commodity, output] : employee_outputs) {
		this->change_total_employee_commodity_output(commodity, output * multiplier);
	}

	if (profession->get_output_commodity()->is_food() && this->is_resource_employment()) {
		//workers employed in resource food production do not need food themselves
		this->get_employment_province()->get_provincial_capital()->get_game_data()->change_free_food_consumption(1 * multiplier);
	}

	const commodity_map<centesimal_int> inputs = this->get_employee_commodity_inputs(employee->get_type(), profession);
	for (const auto &[input_commodity, input_value] : inputs) {
		assert_throw(this->get_employment_country() != nullptr);
		country_economy *country_economy = this->get_employment_country()->get_economy();
		country_economy->change_commodity_input(input_commodity, input_value * multiplier, change_input_storage);
	}

	const int input_wealth = profession->get_input_wealth();
	//FIXME: apply production modifiers to input wealth

	if (input_wealth != 0) {
		assert_throw(this->get_employment_country() != nullptr);
		country_economy *country_economy = this->get_employment_country()->get_economy();

		if (change_input_storage) {
			country_economy->change_wealth(-input_wealth * multiplier);
		}
		country_economy->change_wealth_income(-input_wealth * multiplier);
	}
}

int employment_location::get_production_capacity() const
{
	return this->production_capacity;
}

void employment_location::change_production_capacity(const int change)
{
	if (change == 0) {
		return;
	}

	this->production_capacity += change;

	if (this->get_available_production_capacity() < 0) {
		this->check_excess_employment();
	}
}

const centesimal_int &employment_location::get_employed_production_capacity() const
{
	return this->employed_production_capacity;
}

int employment_location::get_employed_production_capacity_int() const
{
	return this->get_employed_production_capacity().to_int();
}

void employment_location::change_employed_production_capacity(const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	this->employed_production_capacity += change;

	assert_throw(this->get_employed_production_capacity() >= 0);
}

centesimal_int employment_location::get_available_production_capacity() const
{
	return this->get_production_capacity() - this->get_employed_production_capacity();
}

commodity_map<centesimal_int> employment_location::get_employee_commodity_inputs(const population_type *population_type, const profession *profession) const
{
	assert_throw(population_type != nullptr);
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));

	if (profession->get_input_commodities().empty()) {
		return {};
	}

	commodity_map<decimillesimal_int> inputs;

	for (const auto &[commodity, input] : profession->get_input_commodities()) {
		inputs[commodity] += decimillesimal_int(input);
	}

	const int output_modifier = population_type->get_profession_output_modifier(profession);

	for (auto &[commodity, input] : inputs) {
		input *= this->get_employment_output_multiplier();

		if (output_modifier != 0) {
			input *= 100 + output_modifier;
			input /= 100;
		}

		if (this->get_employment_country() != nullptr) {
			const country_economy *country_economy = this->get_employment_country()->get_economy();

			const int throughput_modifier = country_economy->get_throughput_modifier() + country_economy->get_commodity_throughput_modifier(profession->get_output_commodity());
			if (throughput_modifier != 0) {
				input *= 100 + throughput_modifier;
				input /= 100;
			}
		}
	}

	commodity_map<centesimal_int> ret_inputs;
	for (const auto &[commodity, input] : inputs) {
		ret_inputs[commodity] = centesimal_int(input);
	}

	return ret_inputs;
}

commodity_map<centesimal_int> employment_location::get_employee_commodity_outputs(const population_type *population_type, const profession *profession) const
{
	assert_throw(population_type != nullptr);
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));

	commodity_map<decimillesimal_int> outputs;

	const commodity *main_output_commodity = profession->get_output_commodity();
	outputs[main_output_commodity] = this->get_employee_main_commodity_output(population_type, profession);

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

	if (employment_country != nullptr) {
		const country_economy *country_economy = employment_country->get_economy();

		for (auto &[commodity, output] : outputs) {
			const int throughput_modifier = country_economy->get_throughput_modifier() + country_economy->get_commodity_throughput_modifier(commodity);
			if (throughput_modifier != 0) {
				output *= 100 + throughput_modifier;
				output /= 100;
			}
		}
	}

	commodity_map<centesimal_int> ret_outputs;
	for (const auto &[commodity, output] : outputs) {
		ret_outputs[commodity] = centesimal_int(output);
	}

	return ret_outputs;
}

decimillesimal_int employment_location::get_employee_main_commodity_output(const population_type *population_type, const profession *profession) const
{
	assert_throw(population_type != nullptr);
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));

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
		const commodity_map<centesimal_int> employee_outputs = this->get_employee_commodity_outputs(employee->get_type(), employee->get_profession());

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
	while (this->get_available_production_capacity() < 0) {
		const std::vector<const profession *> professions = vector::shuffled(this->get_employment_professions());
		for (const profession *profession : professions) {
			const bool changed = this->decrease_employment(profession, true, std::nullopt);
			if (changed) {
				break;
			}
		}
	}
}

void employment_location::check_superfluous_employment()
{
	if (this->get_employee_count() == 0) {
		return;
	}

	//remove employees who only produce fractional output, if together with others they don't contribute to integer output
	//for example, if the employment location has a single employee who produces 0.5 of a commodity, then this may result superfluous production: inputs will still be consumed, but 0 output will be available for transport (since transport uses integers)

	const std::vector<const profession *> professions = vector::shuffled(this->get_employment_professions());
	for (const profession *profession : professions) {
		centesimal_int main_commodity_output = this->get_total_employee_commodity_outputs().find(profession->get_output_commodity())->second;

		bool changed = true;
		while (this->get_employee_count() > 0 && main_commodity_output.get_fractional_value() > 0 && changed) {
			changed = this->decrease_employment(profession, true, centesimal_int::from_value(main_commodity_output.get_fractional_value()));

			if (changed && this->get_employee_count() > 0) {
				main_commodity_output = this->get_total_employee_commodity_outputs().find(profession->get_output_commodity())->second;
			}
		}
	}
}

bool employment_location::decrease_employment(const profession *profession, const bool change_input_storage, const std::optional<centesimal_int> &max_employee_output_value)
{
	assert_throw(this->get_employee_count() > 0);
	assert_throw(profession != nullptr);
	assert_throw(vector::contains(this->get_employment_professions(), profession));

	std::vector<population_unit *> potential_employees;

	centesimal_int lowest_output_value = centesimal_int::from_value(std::numeric_limits<int64_t>::max());
	if (max_employee_output_value.has_value()) {
		lowest_output_value = max_employee_output_value.value();
	}

	for (population_unit *employee : this->get_employees()) {
		if (employee->get_profession() != profession) {
			continue;
		}

		const centesimal_int output_value = this->get_employee_commodity_outputs(employee->get_type(), profession)[profession->get_output_commodity()];

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
	employee_to_remove->set_employment_location(nullptr, nullptr, change_input_storage);

	return true;
}

}
