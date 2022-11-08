#include "metternich.h"

#include "population/population_class_container.h"

#include "population/population_class.h"

namespace metternich {

bool population_class_compare::operator()(const population_class *lhs, const population_class *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
