#include "metternich.h"

#include "ui/portrait_container.h"

#include "ui/portrait.h"

namespace metternich {

bool portrait_compare::operator()(const portrait *lhs, const portrait *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
