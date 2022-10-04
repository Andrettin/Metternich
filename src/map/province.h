#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class province final : public named_data_entry, public data_type<province>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "province";
	static constexpr const char property_class_identifier[] = "metternich::province*";
	static constexpr const char database_folder[] = "provinces";

	explicit province(const std::string &identifier) : named_data_entry(identifier)
	{
	}
};

}
