#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class arcane_school final : public named_data_entry, public data_type<arcane_school>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "arcane_school";
	static constexpr const char property_class_identifier[] = "metternich::arcane_school*";
	static constexpr const char database_folder[] = "arcane_schools";

	explicit arcane_school(const std::string &identifier) : named_data_entry(identifier)
	{
	}

signals:
	void changed();

private:
};

}
