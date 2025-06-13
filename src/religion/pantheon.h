#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class pantheon final : public named_data_entry, public data_type<pantheon>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "pantheon";
	static constexpr const char property_class_identifier[] = "metternich::pantheon*";
	static constexpr const char database_folder[] = "pantheons";

	explicit pantheon(const std::string &identifier) : named_data_entry(identifier)
	{
	}

signals:
	void changed();

private:
};

}
