#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class divine_domain final : public named_data_entry, public data_type<divine_domain>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "divine_domain";
	static constexpr const char property_class_identifier[] = "metternich::divine_domain*";
	static constexpr const char database_folder[] = "divine_domains";

	explicit divine_domain(const std::string &identifier) : named_data_entry(identifier)
	{
	}

signals:
	void changed();

private:
};

}
