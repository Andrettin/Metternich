#include "metternich.h"

#include "economy/commodity_container.h"

#include "economy/commodity.h"

namespace metternich {

bool commodity_compare::operator()(const commodity *lhs, const commodity *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
