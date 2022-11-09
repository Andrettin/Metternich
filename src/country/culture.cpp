#include "metternich.h"

#include "country/culture.h"

#include "country/cultural_group.h"
#include "population/population_class.h"
#include "util/assert_util.h"

namespace metternich {

void culture::check() const
{
	assert_throw(this->get_group() != nullptr);

	culture_base::check();
}

const population_type *culture::get_population_class_type(const population_class *population_class) const
{
	const population_type *type = culture_base::get_population_class_type(population_class);
	if (type != nullptr) {
		return type;
	}

	if (this->get_group() != nullptr) {
		type = this->get_group()->get_population_class_type(population_class);
		if (type != nullptr) {
			return type;
		}
	}

	return population_class->get_default_population_type();

}

}
