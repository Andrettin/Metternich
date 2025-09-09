#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character_attribute final : public named_data_entry, public data_type<character_attribute>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_attribute";
	static constexpr const char property_class_identifier[] = "metternich::character_attribute*";
	static constexpr const char database_folder[] = "character_attributes";

	explicit character_attribute(const std::string &identifier) : named_data_entry(identifier)
	{
	}

signals:
	void changed();
};

}
