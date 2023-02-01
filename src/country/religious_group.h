#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class religious_group final : public named_data_entry, public data_type<religious_group>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "religious_group";
	static constexpr const char property_class_identifier[] = "metternich::religious_group*";
	static constexpr const char database_folder[] = "religious_groups";

	explicit religious_group(const std::string &identifier) : named_data_entry(identifier)
	{
	}
};

}
