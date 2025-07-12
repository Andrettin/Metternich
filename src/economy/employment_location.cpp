#include "metternich.h"

#include "economy/employment_location.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/profession.h"
#include "util/assert_util.h"

namespace metternich {

const country *employment_location::get_employment_country() const
{
	return this->get_employment_site()->get_game_data()->get_owner();
}

const province *employment_location::get_employment_province() const
{
	return this->get_employment_site()->get_game_data()->get_province();
}

void employment_location::add_employee(population_unit *employee)
{
	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);
	assert_throw(profession->can_employ(employee->get_type()));

	this->employees.push_back(employee);

	this->on_employee_added(employee, 1);

	assert_throw(this->get_available_employment_capacity() >= 0);
}


void employment_location::on_employee_added(population_unit *employee, const int multiplier)
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

commodity_map<centesimal_int> employment_location::get_employee_commodity_outputs(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	commodity_map<decimillesimal_int> outputs;

	const commodity *main_output_commodity = profession->get_output_commodity();
	decimillesimal_int &main_output = outputs[main_output_commodity];

	main_output = decimillesimal_int(profession->get_output_value());
	main_output *= this->get_employment_output_multiplier();

	main_output += decimillesimal_int(population_type->get_profession_output_bonus(profession));

	if (this->is_resource_employment()) {
		main_output += population_type->get_resource_output_bonus();
	}

	const country *employment_country = this->get_employment_country();
	if (employment_country != nullptr) {
		const commodity_map<decimillesimal_int> &commodity_bonuses = employment_country->get_game_data()->get_profession_commodity_bonuses(profession);
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
		assert_throw(this->get_employees().size() > 0);
		this->get_employees().back()->set_employment_location(nullptr);
	}
}

}
