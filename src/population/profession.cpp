#include "metternich.h"

#include "population/profession.h"

#include "economy/commodity.h"
#include "population/population_type.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

void profession::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "input_commodities") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const commodity *commodity = commodity::get(key);
			const int value_int = std::stoi(value);

			this->input_commodities[commodity] = value_int;
		});
	} else if (tag == "employees") {
		for (const std::string &value : values) {
			this->employees.insert(population_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void profession::initialize()
{
	//set whether this production type is industrial
	for (const auto &[commodity, input] : this->get_input_commodities()) {
		if (!commodity->is_abstract() && !commodity->is_labor()) {
			this->industrial = true;
			break;
		}
	}

	named_data_entry::initialize();
}

void profession::check() const
{
	assert_throw(!this->employees.empty());
	assert_throw(this->get_output_commodity() != nullptr);
	assert_throw(this->get_output_value() > 0);
}

QVariantList profession::get_input_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_input_commodities());
}

}
