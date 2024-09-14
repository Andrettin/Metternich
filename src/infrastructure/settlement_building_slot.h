#pragma once

#include "infrastructure/building_slot.h"

Q_MOC_INCLUDE("infrastructure/wonder.h")

namespace metternich {

class employment_type;
class site;
class wonder;

class settlement_building_slot final : public building_slot
{
	Q_OBJECT

	Q_PROPERTY(const metternich::wonder* wonder READ get_wonder NOTIFY wonder_changed)
	Q_PROPERTY(const metternich::wonder* under_construction_wonder READ get_under_construction_wonder WRITE set_under_construction_wonder NOTIFY under_construction_wonder_changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string NOTIFY country_modifier_changed)

public:
	explicit settlement_building_slot(const building_slot_type *type, const site *settlement);

	void set_building(const building_type *building);
	virtual bool can_have_building(const building_type *building) const override;
	virtual bool can_maintain_building(const building_type *building) const override;
	virtual bool can_build_building(const building_type *building) const override;
	void on_building_gained(const building_type *building, const int multiplier);

	const wonder *get_wonder() const
	{
		return this->wonder;
	}

	void set_wonder(const wonder *wonder);

	const wonder *get_under_construction_wonder() const
	{
		return this->under_construction_wonder;
	}

	void set_under_construction_wonder(const wonder *wonder);

	virtual bool can_have_wonder(const wonder *wonder) const override;
	bool can_gain_wonder(const wonder *wonder) const;
	bool can_build_wonder(const wonder *wonder) const;

	void build_wonder(const wonder *wonder);
	Q_INVOKABLE virtual void cancel_construction() override;

	Q_INVOKABLE const metternich::wonder *get_buildable_wonder() const;

	const site *get_settlement() const
	{
		return this->settlement;
	}

	virtual const country *get_country() const override;

	QString get_modifier_string() const;

	const employment_type *get_employment_type() const;

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

	void check_excess_employment();

signals:
	void wonder_changed();
	void under_construction_wonder_changed();
	void country_modifier_changed();

private:
	const site *settlement = nullptr;
	const metternich::wonder *wonder = nullptr;
	const metternich::wonder *under_construction_wonder = nullptr;
	std::vector<population_unit *> employees;
	int employment_capacity = 0;
};

}
