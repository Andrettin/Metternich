#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class building_type;

class building_slot_type final : public named_data_entry, public data_type<building_slot_type>
{
	Q_OBJECT

	Q_PROPERTY(bool coastal MEMBER coastal READ is_coastal NOTIFY changed)
	Q_PROPERTY(bool near_water MEMBER near_water READ is_near_water NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "building_slot_type";
	static constexpr const char property_class_identifier[] = "metternich::building_slot_type*";
	static constexpr const char database_folder[] = "building_slot_types";

public:
	explicit building_slot_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	bool is_near_water() const
	{
		return this->near_water;
	}

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
	bool coastal = false;
	bool near_water = false;
	std::vector<const building_type *> building_types;
};

}
