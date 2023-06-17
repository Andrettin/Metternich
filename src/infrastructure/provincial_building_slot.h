#pragma once

#include "infrastructure/building_slot.h"

namespace metternich {

class province;

class provincial_building_slot final : public building_slot
{
	Q_OBJECT

	Q_PROPERTY(QString modifier_string READ get_modifier_string NOTIFY country_modifier_changed)

public:
	explicit provincial_building_slot(const building_slot_type *type, const metternich::province *province);

	void set_building(const building_type *building);
	virtual bool can_have_building(const building_type *building) const override;

	const metternich::province *get_province() const
	{
		return this->province;
	}

	virtual const country *get_country() const override;

	QString get_modifier_string() const;

signals:
	void country_modifier_changed();

private:
	const metternich::province *province = nullptr;
};

}
