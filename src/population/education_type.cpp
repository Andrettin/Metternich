#include "metternich.h"

#include "population/education_type.h"

#include "economy/commodity.h"
#include "population/population_type.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

void education_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "input_commodities") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const commodity *commodity = commodity::get(key);
			const int value_int = std::stoi(value);

			this->input_commodities[commodity] = value_int;
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void education_type::check() const
{
	assert_throw(this->get_input_population_type() != nullptr);
	assert_throw(!this->get_input_commodities().empty() || this->get_input_wealth() != 0);
	assert_throw(this->get_output_population_type() != nullptr);
}

QVariantList education_type::get_input_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_input_commodities());
}

bool education_type::is_enabled() const
{
	for (const auto &[commodity, input] : this->get_input_commodities()) {
		if (!commodity->is_enabled()) {
			return false;
		}
	}

	if (this->get_input_population_type() != nullptr && !this->get_input_population_type()->is_enabled()) {
		return false;
	}

	if (this->get_output_population_type() != nullptr && !this->get_output_population_type()->is_enabled()) {
		return false;
	}

	return true;
}

}
