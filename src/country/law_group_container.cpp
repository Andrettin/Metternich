#include "metternich.h"

#include "country/law_group_container.h"

#include "country/law_group.h"

namespace metternich {

bool law_group_compare::operator()(const law_group *lhs, const law_group *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
