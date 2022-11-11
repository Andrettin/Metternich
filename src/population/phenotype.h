#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class phenotype final : public named_data_entry, public data_type<phenotype>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "phenotype";
	static constexpr const char property_class_identifier[] = "metternich::phenotype*";
	static constexpr const char database_folder[] = "phenotypes";

	explicit phenotype(const std::string &identifier) : named_data_entry(identifier)
	{
	}
};

}
