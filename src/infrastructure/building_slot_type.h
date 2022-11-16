#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

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
};

}
