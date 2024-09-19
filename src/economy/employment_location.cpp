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

	const centesimal_int employee_output = this->get_employee_output(employee->get_type());
	this->change_total_employee_output(employee_output * multiplier);

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

centesimal_int employment_location::get_employee_output(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	const profession *profession = this->get_employment_profession();
	assert_throw(profession != nullptr);

	const commodity *output_commodity = profession->get_output_commodity();

	centesimal_int employee_output = profession->get_output_value();
	employee_output *= this->get_employment_output_multiplier();

	employee_output += population_type->get_commodity_output_bonus(output_commodity);
	if (this->is_resource_employment()) {
		employee_output += population_type->get_resource_output_bonus();
	}

	const country *employment_country = this->get_employment_country();
	if (employment_country != nullptr) {
		employee_output += employment_country->get_game_data()->get_profession_output_bonus(profession);
	}

	int output_modifier = population_type->get_commodity_output_modifier(output_commodity);
	if (output_modifier != 0) {
		employee_output *= 100 + output_modifier;
		employee_output /= 100;
	}

	return employee_output;
}

void employment_location::change_total_employee_output(const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	this->total_employee_output += change;

	this->get_employment_site()->get_game_data()->change_base_commodity_output(this->get_employment_profession()->get_output_commodity(), change);
}

void employment_location::calculate_total_employee_output()
{
	centesimal_int old_output = this->get_total_employee_output();

	centesimal_int new_output;
	for (const population_unit *employee : this->get_employees()) {
		new_output += this->get_employee_output(employee->get_type());
	}

	const centesimal_int output_change = new_output - old_output;
	this->change_total_employee_output(output_change);
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
