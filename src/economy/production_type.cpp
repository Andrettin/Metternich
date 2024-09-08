#include "metternich.h"

#include "economy/production_type.h"

#include "economy/commodity.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

void production_type::process_gsml_scope(const gsml_data &scope)
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

void production_type::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_production_type(this);
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

void production_type::check() const
{
	assert_throw(!this->get_input_commodities().empty() || this->get_input_wealth() != 0);
	assert_throw(this->get_output_commodity() != nullptr);
	assert_throw(this->get_output_value() > 0);
}

QVariantList production_type::get_input_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_input_commodities());
}

}
