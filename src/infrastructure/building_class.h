#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class building_type;

class building_class final : public named_data_entry, public data_type<building_class>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "building_class";
	static constexpr const char property_class_identifier[] = "metternich::building_class*";
	static constexpr const char database_folder[] = "building_classes";

public:
	explicit building_class(const std::string &identifier) : named_data_entry(identifier)
	{
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

private:
	const building_type *default_building_type = nullptr;
	std::vector<const building_type *> building_types;
};

}
