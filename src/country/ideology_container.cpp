#include "metternich.h"

#include "country/ideology_container.h"

#include "country/ideology.h"

namespace metternich {

bool ideology_compare::operator()(const ideology *lhs, const ideology *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
