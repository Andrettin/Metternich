#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class employment_type;

class employment_capacity_type final : public named_data_entry, public data_type<employment_capacity_type>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "employment_capacity_type";
	static constexpr const char property_class_identifier[] = "metternich::employment_capacity_type*";
	static constexpr const char database_folder[] = "employment_capacity_types";

	explicit employment_capacity_type(const std::string &identifier);
	~employment_capacity_type();

	virtual void check() const override;

	const std::vector<const employment_type *> &get_employment_types() const
	{
		return this->employment_types;
	}

	void add_employment_type(const employment_type *employment_type)
	{
		this->employment_types.push_back(employment_type);
	}

signals:
	void changed();

private:
	std::vector<const employment_type *> employment_types;
};

}
