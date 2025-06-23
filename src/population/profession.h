#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class population_type;

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

	const std::vector<const population_type *> &get_population_types() const
	{
		return this->population_types;
	}

	void add_population_type(const population_type *population_type)
	{
		this->population_types.push_back(population_type);
	}

private:
	std::vector<const population_type *> population_types;
};

}
