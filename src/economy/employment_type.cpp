#include "metternich.h"

#include "economy/employment_type.h"

#include "economy/commodity.h"
#include "population/population_class.h"
#include "util/assert_util.h"

namespace metternich {

void employment_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "input_commodities") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const commodity *commodity = commodity::get(key);
			const centesimal_int multiplier(value);

			this->input_commodities[commodity] = multiplier;
		});
	} else if (tag == "employees") {
		for (const std::string &value : values) {
			this->employees.push_back(population_class::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void employment_type::check() const
{
	assert_throw(this->get_output_commodity() != nullptr);
}

}
