#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class population_type;

class population_class final : public named_data_entry, public data_type<population_class>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "population_class";
	static constexpr const char property_class_identifier[] = "metternich::population_class*";
	static constexpr const char database_folder[] = "population_classes";

public:
	explicit population_class(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const population_type *get_default_population_type() const
	{
		return this->default_population_type;
	}

	void set_default_population_type(const population_type *population_type);

	const std::vector<const population_type *> &get_population_types() const
	{
		return this->population_types;
	}

	void add_population_type(const population_type *population_type)
	{
		this->population_types.push_back(population_type);
	}

private:
	const population_type *default_population_type = nullptr;
	std::vector<const population_type *> population_types;
};

}
