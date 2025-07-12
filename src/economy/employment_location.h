#pragma once

#include "economy/commodity_container.h"
#include "util/fractional_int.h"

namespace metternich {

class country;
class population_type;
class population_unit;
class profession;
class province;
class site;

class employment_location
{
public:
	virtual const site *get_employment_site() const = 0;
	const province *get_employment_province() const;
	const country *get_employment_country() const;
	virtual const profession *get_employment_profession() const = 0;

	virtual bool is_resource_employment() const
	{
		return false;
	}

	virtual int get_employment_output_multiplier() const
	{
		return 1;
	}

	int get_employee_count() const
	{
		return static_cast<int>(this->get_employees().size());
	}

	const std::vector<population_unit *> &get_employees() const
	{
		return this->employees;
	}

	void add_employee(population_unit *employee);

	void remove_employee(population_unit *employee)
	{
		std::erase(this->employees, employee);

		this->on_employee_added(employee, -1);
	}

	void on_employee_added(population_unit *employee, const int multiplier);

	int get_employment_capacity() const
	{
		return this->employment_capacity;
	}

	void change_employment_capacity(const int change);

	int get_available_employment_capacity() const
	{
		return this->get_employment_capacity() - this->get_employee_count();
	}

	commodity_map<centesimal_int> get_employee_commodity_outputs(const population_type *population_type) const;

	const commodity_map<centesimal_int> &get_total_employee_commodity_outputs() const
	{
		return this->total_employee_commodity_outputs;
	}

	void change_total_employee_commodity_output(const commodity *commodity, const centesimal_int &change);
	void calculate_total_employee_commodity_outputs();

	void check_excess_employment();

private:
	std::vector<population_unit *> employees;
	int employment_capacity = 0;
	commodity_map<centesimal_int> total_employee_commodity_outputs;
};

}
