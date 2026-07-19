#include "metternich.h"

#include "map/route_type.h"

#include "economy/commodity.h"
#include "util/assert_util.h"

namespace metternich {

void route_type::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "output_multiplier") {
		assert_throw(this->get_output_commodity() != nullptr);
		this->output_multiplier = this->get_output_commodity()->string_to_value(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void route_type::check() const
{
	if (this->get_output_commodity() == nullptr) {
		throw std::runtime_error(std::format("Route type \"{}\" has no output commodity.", this->get_identifier()));
	}

	if (this->get_output_multiplier() == 0) {
		throw std::runtime_error(std::format("Route type \"{}\" has no output multiplier.", this->get_identifier()));
	}

	named_data_entry::check();
}

}
