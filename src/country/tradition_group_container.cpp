#include "metternich.h"

#include "country/tradition_group_container.h"

#include "country/tradition_group.h"

namespace metternich {

bool tradition_group_compare::operator()(const tradition_group *lhs, const tradition_group *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
