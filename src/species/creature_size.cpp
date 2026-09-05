#include "metternich.h"

#include "species/creature_size.h"

#include "species/creature_size_container.h"
#include "script/modifier.h"
#include "util/string_conversion_util.h"

namespace metternich {
	
void creature_size::initialize_all()
{
	data_type::initialize_all();

	creature_size::sort_instances(creature_size_compare());
}

creature_size::creature_size(const std::string &identifier)
	: named_data_entry(identifier)
{
}

creature_size::~creature_size()
{
}

void creature_size::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "min_dimension") {
		this->min_dimension = string::to_length(value);
	} else if (key == "max_dimension") {
		this->max_dimension = string::to_length(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void creature_size::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void creature_size::check() const
{
	if (this->get_min_dimension() == 0 && this->get_max_dimension() == 0) {
		throw std::runtime_error(std::format("Creature size \"{}\" has neither a minimum nor a maximum dimension.", this->get_identifier()));
	}

	named_data_entry::check();
}

}
