#pragma once

#include "economy/commodity_container.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("infrastructure/building_type.h")

namespace metternich {

class building_slot_type;
class building_type;
class country;

class building_slot : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_slot_type* type READ get_type_unconst CONSTANT)
	Q_PROPERTY(metternich::building_type* building READ get_building_unconst NOTIFY building_changed)
	Q_PROPERTY(metternich::building_type* under_construction_building READ get_under_construction_building_unconst WRITE set_under_construction_building NOTIFY under_construction_building_changed)
	Q_PROPERTY(metternich::country* country READ get_country_unconst CONSTANT)

public:
	explicit building_slot(const building_slot_type *type);

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

protected:
	void set_building(const building_type *building);

public:
	const building_type *get_under_construction_building() const
	{
		return this->under_construction_building;
	}

private:
	//for the Qt property (pointers there can't be const)
	building_type *get_under_construction_building_unconst() const
	{
		return const_cast<building_type *>(this->get_under_construction_building());
	}

public:
	void set_under_construction_building(const building_type *building);

	virtual bool can_have_building(const building_type *building) const;
	virtual bool can_maintain_building(const building_type *building) const;
	virtual bool can_gain_building(const building_type *building) const;
	virtual bool can_build_building(const building_type *building) const;

	void build_building(const building_type *building);

	Q_INVOKABLE void build_building(metternich::building_type *building)
	{
		const metternich::building_type *const_building = building;
		this->build_building(const_building);
	}

	Q_INVOKABLE virtual void cancel_construction();

	Q_INVOKABLE metternich::building_type *get_buildable_building() const;

	virtual const metternich::country *get_country() const = 0;

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_country_unconst() const
	{
		return const_cast<metternich::country *>(this->get_country());
	}

public:
	bool is_available() const;

signals:
	void building_changed();
	void under_construction_building_changed();

private:
	const building_slot_type *type = nullptr;
	const building_type *building = nullptr;
	const building_type *under_construction_building = nullptr;
};

}
