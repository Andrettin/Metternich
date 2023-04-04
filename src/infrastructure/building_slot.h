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
	Q_PROPERTY(int capacity READ get_capacity NOTIFY building_changed)
	Q_PROPERTY(QVariantList available_production_types READ get_available_production_types_qvariant_list NOTIFY building_changed)

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

	std::vector<const production_type *> get_available_production_types() const;
	QVariantList get_available_production_types_qvariant_list() const;

	int get_production_type_employed_capacity(const production_type *production_type) const
	{
		const auto find_iterator = this->production_type_employed_capacities.find(production_type);
		if (find_iterator != this->production_type_employed_capacities.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_production_type_employed_capacity(metternich::production_type *production_type) const
	{
		const metternich::production_type *const_production_type = production_type;
		return this->get_production_type_employed_capacity(const_production_type);
	}

	int get_production_type_output(const production_type *production_type) const;

	Q_INVOKABLE int get_production_type_output(metternich::production_type *production_type) const
	{
		const metternich::production_type *const_production_type = production_type;
		return this->get_production_type_output(const_production_type);
	}

	void change_production(const production_type *production_type, const int change, const bool change_input_storage = true);

	bool can_increase_production(const production_type *production_type) const;

	Q_INVOKABLE bool can_increase_production(metternich::production_type *production_type) const
	{
		const metternich::production_type *const_production_type = production_type;
		return this->can_increase_production(const_production_type);
	}

	void increase_production(const production_type *production_type);

	Q_INVOKABLE void increase_production(metternich::production_type *production_type)
	{
		const metternich::production_type *const_production_type = production_type;
		this->increase_production(const_production_type);
	}

	bool can_decrease_production(const production_type *production_type) const;

	Q_INVOKABLE bool can_decrease_production(metternich::production_type *production_type) const
	{
		const metternich::production_type *const_production_type = production_type;
		return this->can_decrease_production(const_production_type);
	}

	void decrease_production(const production_type *production_type, const bool restore_inputs);

	Q_INVOKABLE void decrease_production(metternich::production_type *production_type)
	{
		const metternich::production_type *const_production_type = production_type;
		this->decrease_production(const_production_type, true);
	}

	void apply_country_modifier(const metternich::country *country, const int multiplier);

signals:
	void building_changed();
	void employed_capacity_changed();

private:
	const building_slot_type *type = nullptr;
	const building_type *building = nullptr;
	const metternich::country *country = nullptr;
	int employed_capacity = 0;
	std::map<const production_type *, int> production_type_employed_capacities;
};

}
