#include "metternich.h"

#include "technology/technology_container.h"

#include "technology/technology.h"

namespace metternich {

bool technology_compare::operator()(const technology *lhs, const technology *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
