#pragma once

#include "economy/commodity_container.h"
#include "population/profession_container.h"
#include "util/centesimal_int.h"
#include "util/decimillesimal_int.h"

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
	virtual const std::vector<const profession *> &get_employment_professions() const = 0;

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

	QVariantList get_employees_qvariant_list() const;
	profession_map<std::vector<population_unit *>> take_employees();

	bool can_employ(const population_unit *population_unit, const profession *profession, const population_type *&converted_population_type) const;
	bool can_fulfill_inputs_for_employment(const population_unit *population_unit, const profession *profession) const;
	void add_employee(population_unit *employee, const profession *profession);
	void add_employees_if_possible(const profession_map<std::vector<population_unit *>> &employees_by_profession);

	void remove_employee(population_unit *employee, const profession *profession, const bool change_input_storage)
	{
		std::erase(this->employees, employee);

		this->on_employee_added(employee, profession, -1, change_input_storage);
	}

	void on_employee_added(population_unit *employee, const profession *profession, const int multiplier, const bool change_input_storage);

	int get_production_capacity() const;
	void change_production_capacity(const int change);
	const centesimal_int &get_employed_production_capacity() const;
	int get_employed_production_capacity_int() const;
	void change_employed_production_capacity(const centesimal_int &change);
	int get_effective_production_capacity() const;
	centesimal_int get_available_production_capacity() const;

	commodity_map<centesimal_int> get_employee_commodity_inputs(const population_type *population_type, const profession *profession) const;
	commodity_map<centesimal_int> get_employee_commodity_outputs(const population_type *population_type, const profession *profession) const;
	decimillesimal_int get_employee_main_commodity_output(const population_type *population_type, const profession *profession) const;

	const commodity_map<centesimal_int> &get_total_employee_commodity_outputs() const
	{
		return this->total_employee_commodity_outputs;
	}

	void change_total_employee_commodity_output(const commodity *commodity, const centesimal_int &change);
	void calculate_total_employee_commodity_outputs();

	void check_excess_employment();
	void check_superfluous_employment();
	bool decrease_employment(const profession *profession, const bool change_input_storage, const std::optional<centesimal_int> &max_employee_output_value);

private:
	std::vector<population_unit *> employees;
	int production_capacity = 0;
	centesimal_int employed_production_capacity;
	commodity_map<centesimal_int> total_employee_commodity_outputs;
};

}
