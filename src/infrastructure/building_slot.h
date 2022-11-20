#pragma once

namespace metternich {

class building_slot_type;
class building_type;
class population_unit;
class province;

class building_slot final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_type* building READ get_building_unconst NOTIFY building_changed)
	Q_PROPERTY(metternich::province* province READ get_province_unconst CONSTANT)

public:
	explicit building_slot(const building_slot_type *type, const metternich::province *province);

	const building_slot_type *get_type() const
	{
		return this->type;
	}

	const building_type *get_building() const
	{
		return this->building;
	}

private:
	//for the Qt property (pointers there can't be const)
	building_type *get_building_unconst() const
	{
		return const_cast<building_type *>(this->get_building());
	}

public:
	void set_building(const building_type *building);

	const metternich::province *get_province() const
	{
		return this->province;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::province *get_province_unconst() const
	{
		return const_cast<metternich::province *>(this->get_province());
	}

public:
	const std::vector<population_unit *> &get_employees() const
	{
		return this->employees;
	}

	void clear_employees()
	{
		this->employees.clear();
	}

	void add_employee(population_unit *population_unit)
	{
		this->employees.push_back(population_unit);
	}

	void remove_employee(population_unit *population_unit)
	{
		std::erase(this->employees, population_unit);
	}

	int get_employee_count() const
	{
		return static_cast<int>(this->get_employees().size());
	}

	int get_employment_capacity() const;

signals:
	void building_changed();

private:
	const building_slot_type *type = nullptr;
	const building_type *building = nullptr;
	const metternich::province *province = nullptr;
	std::vector<population_unit *> employees;
};

}
