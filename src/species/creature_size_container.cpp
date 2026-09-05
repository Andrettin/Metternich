#include "metternich.h"

#include "species/creature_size_container.h"

#include "species/creature_size.h"

namespace metternich {

bool creature_size_compare::operator()(const creature_size *lhs, const creature_size *rhs) const
{
	if (lhs->get_min_dimension() != rhs->get_min_dimension()) {
		return lhs->get_min_dimension() < rhs->get_min_dimension();
	}

	if (lhs->get_max_dimension() != rhs->get_max_dimension()) {
		return lhs->get_max_dimension() > rhs->get_max_dimension();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
