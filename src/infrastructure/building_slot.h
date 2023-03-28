#pragma once

#include "economy/commodity_container.h"

namespace archimedes {
	template <int N>
	class fractional_int;

	using centesimal_int = fractional_int<2>;
}

namespace metternich {

class building_slot_type;
class building_type;
class country;
class production_type;

class building_slot final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_slot_type* type READ get_type_unconst CONSTANT)
	Q_PROPERTY(metternich::building_type* building READ get_building_unconst NOTIFY building_changed)
	Q_PROPERTY(metternich::country* country READ get_country_unconst CONSTANT)

public:
	explicit building_slot(const building_slot_type *type, const metternich::country *country);

	const building_slot_type *get_type() const
	{
		return this->type;
	}

private:
	//for the Qt property (pointers there can't be const)
	building_slot_type *get_type_unconst() const
	{
		return const_cast<building_slot_type *>(this->get_type());
	}

public:
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

	bool can_have_building(const building_type *building) const;

	const metternich::country *get_country() const
	{
		return this->country;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_country_unconst() const
	{
		return const_cast<metternich::country *>(this->get_country());
	}

public:
	int get_capacity() const;

	int get_employed_capacity() const
	{
		return this->employed_capacity;
	}

	bool can_increase_production(const production_type *production_type) const;
	void increase_production(const production_type *production_type);

	bool can_decrease_production(const production_type *production_type) const;
	void decrease_production(const production_type *production_type);

	void apply_country_modifier(const metternich::country *country, const int multiplier);

signals:
	void building_changed();

private:
	const building_slot_type *type = nullptr;
	const building_type *building = nullptr;
	const metternich::country *country = nullptr;
	int employed_capacity = 0;
	std::map<const production_type *, int> production_type_employed_capacities;
};

}
