#include "metternich.h"

#include "population/population_class.h"

#include "population/population_type.h"
#include "util/assert_util.h"

namespace metternich {

void population_class::set_default_population_type(const population_type *population_type)
{
	if (this->get_default_population_type() != nullptr) {
		throw std::runtime_error(std::format("Cannot set \"{}\" as the default population type of class \"{}\", as it already has \"{}\" as its default type.", population_type->get_identifier(), this->get_identifier(), this->get_default_population_type()->get_identifier()));
	}

	this->default_population_type = population_type;
}

}
