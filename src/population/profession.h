#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class profession final : public named_data_entry, public data_type<profession>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "profession";
	static constexpr const char property_class_identifier[] = "metternich::profession*";
	static constexpr const char database_folder[] = "professions";

public:
	explicit profession(const std::string &identifier) : named_data_entry(identifier)
	{
	}
};

}
