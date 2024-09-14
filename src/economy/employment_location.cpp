#include "metternich.h"

#include "economy/employment_location.h"

#include "economy/employment_type.h"
#include "population/population_unit.h"
#include "util/assert_util.h"

namespace metternich {

void employment_location::add_employee(population_unit *employee)
{
	const employment_type *employment_type = this->get_employment_type();
	assert_throw(employment_type != nullptr);
	assert_throw(employment_type->can_employ(employee->get_type()));

	this->employees.push_back(employee);

	this->on_employee_added(employee, 1);

	assert_throw(this->get_available_employment_capacity() >= 0);
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

void employment_location::check_excess_employment()
{
	//remove employees in excess of capacity
	while (this->get_available_employment_capacity() < 0) {
		assert_throw(this->get_employees().size() > 0);
		this->get_employees().back()->set_employment_location(nullptr);
	}
}

}
