#include "metternich.h"

#include "character/office_container.h"

#include "character/office.h"

namespace metternich {

bool office_compare::operator()(const office *lhs, const office *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
