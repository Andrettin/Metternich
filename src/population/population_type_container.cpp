#include "metternich.h"

#include "population/population_type_container.h"

#include "population/population_type.h"

namespace metternich {

bool population_type_compare::operator()(const population_type *lhs, const population_type *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
