#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class building_type;
class wonder;

class building_slot_type final : public named_data_entry, public data_type<building_slot_type>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "building_slot_type";
	static constexpr const char property_class_identifier[] = "metternich::building_slot_type*";
	static constexpr const char database_folder[] = "building_slot_types";

public:
	explicit building_slot_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const std::vector<const building_type *> &get_building_types() const
	{
		return this->building_types;
	}

	void add_building_type(const building_type *building_type)
	{
		this->building_types.push_back(building_type);
	}

	const std::vector<const wonder *> &get_wonders() const
	{
		return this->wonders;
	}

	void add_wonder(const wonder *wonder)
	{
		this->wonders.push_back(wonder);
	}

signals:
	void changed();

private:
	std::vector<const building_type *> building_types;
	std::vector<const wonder *> wonders;
};

}
