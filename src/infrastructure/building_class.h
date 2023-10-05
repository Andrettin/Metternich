#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("infrastructure/building_slot_type.h")

namespace metternich {

class building_slot_type;
class building_type;

class building_class final : public named_data_entry, public data_type<building_class>
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_slot_type* slot_type MEMBER slot_type NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "building_class";
	static constexpr const char property_class_identifier[] = "metternich::building_class*";
	static constexpr const char database_folder[] = "building_classes";

public:
	explicit building_class(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	building_slot_type *get_slot_type() const
	{
		return this->slot_type;
	}

	const building_type *get_default_building_type() const
	{
		return this->default_building_type;
	}

	void set_default_building_type(const building_type *building_type);

	const std::vector<const building_type *> &get_building_types() const
	{
		return this->building_types;
	}

	void add_building_type(const building_type *building_type)
	{
		this->building_types.push_back(building_type);
	}

signals:
	void changed();

private:
	building_slot_type *slot_type = nullptr;
	const building_type *default_building_type = nullptr;
	std::vector<const building_type *> building_types;
};

}
