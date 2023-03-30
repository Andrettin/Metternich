#include "metternich.h"

#include "economy/commodity_container.h"

#include "economy/commodity.h"

namespace metternich {

bool commodity_compare::operator()(const commodity *lhs, const commodity *rhs) const
{
	if (lhs->is_storable() != rhs->is_storable()) {
		return !lhs->is_storable();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
