#include "metternich.h"

#include "population/profession.h"

#include "economy/commodity.h"
#include "population/population_type.h"
#include "technology/technology.h"
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

			this->input_commodities[commodity] = centesimal_int(value);
		});
	} else if (tag == "population_types") {
		for (const std::string &value : values) {
			this->population_types.insert(population_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void profession::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_profession(this);
	}

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
	assert_throw(!this->population_types.empty());
	assert_throw(this->get_output_commodity() != nullptr);
	assert_throw(this->get_output_value() > 0);
}

QVariantList profession::get_input_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_input_commodities());
}

bool profession::can_employ_with_conversion(const population_type *population_type, const metternich::population_type *&converted_population_type) const
{
	converted_population_type = nullptr;

	if (this->can_employ(population_type)) {
		return true;
	}

	for (const metternich::population_type *equivalent_population_type : population_type->get_equivalent_population_types()) {
		if (this->can_employ(equivalent_population_type)) {
			converted_population_type = equivalent_population_type;
			return true;
		}
	}

	return false;
}

}
