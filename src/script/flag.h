#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class flag final : public named_data_entry, public data_type<flag>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "flag";
	static constexpr const char property_class_identifier[] = "metternich::flag*";
	static constexpr const char database_folder[] = "flags";

	explicit flag(const std::string &identifier) : named_data_entry(identifier)
	{
	}
};

}
