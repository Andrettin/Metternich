#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class military_unit_type;
enum class military_unit_domain;

class military_unit_class final : public named_data_entry, public data_type<military_unit_class>
{
	Q_OBJECT

	Q_PROPERTY(metternich::military_unit_domain domain MEMBER domain READ get_domain)

public:
	static constexpr const char *class_identifier = "military_unit_class";
	static constexpr const char property_class_identifier[] = "metternich::military_unit_class*";
	static constexpr const char *database_folder = "unit_classes";

public:
	explicit military_unit_class(const std::string &identifier);

	military_unit_domain get_domain() const
	{
		return this->domain;
	}

	const std::vector<const military_unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}

	void add_unit_type(const military_unit_type *unit_type)
	{
		this->unit_types.push_back(unit_type);
	}

private:
	military_unit_domain domain;
	std::vector<const military_unit_type *> unit_types;
};

}