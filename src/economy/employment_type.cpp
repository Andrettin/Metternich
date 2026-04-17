#include "metternich.h"

#include "economy/employment_type.h"

#include "population/population_type.h"

namespace metternich {
	
employment_type::employment_type(const std::string &identifier) : named_data_entry(identifier)
{
}

employment_type::~employment_type()
{
}

void employment_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "employee_types") {
		for (const std::string &value : values) {
			this->employee_types.insert(population_type::get(value));
		}
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void employment_type::check() const
{
	if (this->get_employee_types().empty()) {
		throw std::runtime_error(std::format("Employment type \"{}\" has no employee types.", this->get_identifier()));
	}
}

}
