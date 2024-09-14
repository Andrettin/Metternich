#pragma once

namespace metternich {

class employment_type;
class population_type;
class population_unit;

class employment_location
{
public:
	virtual const employment_type *get_employment_type() const = 0;

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

	virtual void on_employee_added(population_unit *employee, const int multiplier) = 0;

	int get_employment_capacity() const
	{
		return this->employment_capacity;
	}

	void change_employment_capacity(const int change);

	int get_available_employment_capacity() const
	{
		return this->get_employment_capacity() - this->get_employee_count();
	}

	virtual centesimal_int get_employee_output(const population_type *population_type) const = 0;

	void check_excess_employment();

private:
	std::vector<population_unit *> employees;
	int employment_capacity = 0;
};

}
