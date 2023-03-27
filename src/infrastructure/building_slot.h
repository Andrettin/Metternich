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
class population_unit;
class province;

class building_slot final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_slot_type* type READ get_type_unconst CONSTANT)
	Q_PROPERTY(metternich::building_type* building READ get_building_unconst NOTIFY building_changed)
	Q_PROPERTY(metternich::province* province READ get_province_unconst CONSTANT)
	Q_PROPERTY(bool available READ is_available NOTIFY available_changed)

public:
	explicit building_slot(const building_slot_type *type, const metternich::province *province);

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
	const country *get_country() const;

	bool is_available() const;

	int get_capacity() const;

	const commodity_map<centesimal_int> &get_base_commodity_outputs() const
	{
		return this->base_commodity_outputs;
	}

	commodity_map<centesimal_int> get_commodity_outputs() const;

	void calculate_base_commodity_outputs();

	void apply_country_modifier(const country *country, const int multiplier);

signals:
	void building_changed();
	void available_changed();

private:
	const building_slot_type *type = nullptr;
	const building_type *building = nullptr;
	const metternich::province *province = nullptr;
	commodity_map<centesimal_int> base_commodity_outputs;
};

}
