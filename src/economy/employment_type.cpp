#include "metternich.h"

#include "economy/employment_type.h"

#include "economy/commodity.h"
#include "population/population_type.h"
#include "util/assert_util.h"

namespace metternich {
	
employment_type::employment_type(const std::string &identifier) : named_data_entry(identifier)
{
}

employment_type::~employment_type()
{
}

void employment_type::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "monthly_output_value") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		assert_throw(this->get_output_commodity() != nullptr);
		this->monthly_output_value = this->get_output_commodity()->string_to_value(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
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

	if (this->get_output_commodity() != nullptr && this->get_base_employment_size() == 0) {
		throw std::runtime_error(std::format("Employment type \"{}\" has an output commodity, but no base employment size.", this->get_identifier()));
	}
}

}
