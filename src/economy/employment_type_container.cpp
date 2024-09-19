#include "metternich.h"

#include "economy/employment_type_container.h"

#include "economy/employment_type.h"

namespace metternich {

bool employment_type_compare::operator()(const employment_type *lhs, const employment_type *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
