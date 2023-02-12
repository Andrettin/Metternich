#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class civilian_unit_type;

class civilian_unit_class final : public named_data_entry, public data_type<civilian_unit_class>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "civilian_unit_class";
	static constexpr const char property_class_identifier[] = "metternich::civilian_unit_class*";
	static constexpr const char database_folder[] = "civilian_unit_classes";

public:
	explicit civilian_unit_class(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const civilian_unit_type *get_default_unit_type() const
	{
		return this->default_unit_type;
	}

	void set_default_unit_type(const civilian_unit_type *unit_type);

	const std::vector<const civilian_unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}

	void add_unit_type(const civilian_unit_type *unit_type)
	{
		this->unit_types.push_back(unit_type);
	}

private:
	const civilian_unit_type *default_unit_type = nullptr;
	std::vector<const civilian_unit_type *> unit_types;
};

}
