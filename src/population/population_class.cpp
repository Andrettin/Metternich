#include "metternich.h"

#include "population/population_class.h"

#include "util/assert_util.h"

namespace metternich {

void population_class::set_default_population_type(const population_type *population_type)
{
	assert_throw(this->get_default_population_type() == nullptr);

	this->default_population_type = population_type;
}

}
