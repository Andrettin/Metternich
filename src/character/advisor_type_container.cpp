#include "metternich.h"

#include "character/advisor_type_container.h"

#include "character/advisor_type.h"

namespace metternich {

bool advisor_type_compare::operator()(const advisor_type *lhs, const advisor_type *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
