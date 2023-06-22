#include "metternich.h"

#include "map/route_container.h"

#include "map/route.h"

namespace metternich {

bool route_compare::operator()(const route *lhs, const route *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
