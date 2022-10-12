#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class cultural_group final : public named_data_entry, public data_type<cultural_group>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "cultural_group";
	static constexpr const char property_class_identifier[] = "metternich::cultural_group*";
	static constexpr const char database_folder[] = "cultural_groups";

	explicit cultural_group(const std::string &identifier) : named_data_entry(identifier)
	{
	}
};

}
