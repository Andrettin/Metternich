#include "metternich.h"

#include "technology/technology_container.h"

#include "technology/technology.h"

namespace metternich {

bool technology_compare::operator()(const technology *lhs, const technology *rhs) const
{
	const int lhs_prerequisite_depth = lhs->get_total_prerequisite_depth();
	const int rhs_prerequisite_depth = rhs->get_total_prerequisite_depth();

	if (lhs_prerequisite_depth != rhs_prerequisite_depth) {
		return lhs_prerequisite_depth < rhs_prerequisite_depth;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
