#pragma once

#include "infrastructure/building_slot.h"

namespace metternich {

class province;
class wonder;

class settlement_building_slot final : public building_slot
{
	Q_OBJECT

	Q_PROPERTY(metternich::wonder* wonder READ get_wonder_unconst NOTIFY wonder_changed)
	Q_PROPERTY(metternich::wonder* under_construction_wonder READ get_under_construction_wonder_unconst WRITE set_under_construction_wonder NOTIFY under_construction_wonder_changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string NOTIFY country_modifier_changed)

public:
	explicit settlement_building_slot(const building_slot_type *type, const metternich::province *province);

	void set_building(const building_type *building);
	virtual bool can_have_building(const building_type *building) const override;

public:
	const wonder *get_wonder() const
	{
		return this->wonder;
	}

private:
	//for the Qt property (pointers there can't be const)
	wonder *get_wonder_unconst() const
	{
		return const_cast<metternich::wonder *>(this->get_wonder());
	}

public:
	void set_wonder(const wonder *wonder);

	const wonder *get_under_construction_wonder() const
	{
		return this->under_construction_wonder;
	}

private:
	//for the Qt property (pointers there can't be const)
	wonder *get_under_construction_wonder_unconst() const
	{
		return const_cast<metternich::wonder *>(this->get_under_construction_wonder());
	}

public:
	void set_under_construction_wonder(const wonder *wonder);

	bool can_have_wonder(const wonder *wonder) const;
	bool can_build_wonder(const wonder *wonder) const;

	void build_wonder(const wonder *wonder);
	Q_INVOKABLE virtual void cancel_construction() override;

	Q_INVOKABLE metternich::wonder *get_buildable_wonder() const;

	const metternich::province *get_province() const
	{
		return this->province;
	}

	virtual const country *get_country() const override;

	QString get_modifier_string() const;

signals:
	void wonder_changed();
	void under_construction_wonder_changed();
	void country_modifier_changed();

private:
	const metternich::province *province = nullptr;
	const metternich::wonder *wonder = nullptr;
	const metternich::wonder *under_construction_wonder = nullptr;
};

}
