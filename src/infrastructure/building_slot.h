#pragma once

#include "economy/commodity_container.h"

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("infrastructure/building_type.h")
Q_MOC_INCLUDE("infrastructure/wonder.h")

namespace metternich {

class building_slot_type;
class building_type;
class domain;
class wonder;

class building_slot final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::building_slot_type* type READ get_type CONSTANT)
	Q_PROPERTY(const metternich::building_type* building READ get_building NOTIFY building_changed)
	Q_PROPERTY(const metternich::building_type* under_construction_building READ get_under_construction_building WRITE set_under_construction_building NOTIFY under_construction_building_changed)
	Q_PROPERTY(const metternich::wonder* wonder READ get_wonder NOTIFY wonder_changed)
	Q_PROPERTY(const metternich::wonder* under_construction_wonder READ get_under_construction_wonder WRITE set_under_construction_wonder NOTIFY under_construction_wonder_changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string NOTIFY domain_modifier_changed)
	Q_PROPERTY(const metternich::domain* country READ get_country CONSTANT)

public:
	explicit building_slot(const building_slot_type *type, const site *settlement);

	const building_slot_type *get_type() const
	{
		return this->type;
	}

	const site *get_settlement() const
	{
		return this->settlement;
	}

	const building_type *get_building() const
	{
		return this->building;
	}

	void set_building(const building_type *building);

	const building_type *get_under_construction_building() const
	{
		return this->under_construction_building;
	}

	void set_under_construction_building(const building_type *building);

	bool can_have_building(const building_type *building) const;
	bool can_maintain_building(const building_type *building) const;
	bool can_gain_building(const building_type *building) const;
	bool can_build_building(const building_type *building) const;
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

	bool can_have_wonder(const wonder *wonder) const;
	bool can_gain_wonder(const wonder *wonder) const;
	bool can_build_wonder(const wonder *wonder) const;

	void build_wonder(const wonder *wonder);

	Q_INVOKABLE void build_building(const metternich::building_type *building);
	Q_INVOKABLE virtual void cancel_construction();
	Q_INVOKABLE const metternich::building_type *get_buildable_building() const;
	Q_INVOKABLE const metternich::wonder *get_buildable_wonder() const;

	const metternich::domain *get_country() const;

	bool is_available() const;

	QString get_modifier_string() const;

signals:
	void building_changed();
	void under_construction_building_changed();
	void wonder_changed();
	void under_construction_wonder_changed();
	void domain_modifier_changed();

private:
	const building_slot_type *type = nullptr;
	const site *settlement = nullptr;
	const building_type *building = nullptr;
	const building_type *under_construction_building = nullptr;
	const metternich::wonder *wonder = nullptr;
	const metternich::wonder *under_construction_wonder = nullptr;
};

}
