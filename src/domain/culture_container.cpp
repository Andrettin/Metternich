#include "metternich.h"

#include "domain/culture_container.h"

#include "domain/culture.h"

namespace metternich {

bool culture_compare::operator()(const culture *lhs, const culture *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
