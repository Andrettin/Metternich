#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "population/population_type_container.h"

namespace metternich {

class employment_type final : public named_data_entry, public data_type<employment_type>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "employment_type";
	static constexpr const char property_class_identifier[] = "metternich::employment_type*";
	static constexpr const char database_folder[] = "employment_types";

	explicit employment_type(const std::string &identifier);
	~employment_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const population_type_set &get_employee_types() const
	{
		return this->employee_types;
	}

signals:
	void changed();

private:
	population_type_set employee_types;
};

}
