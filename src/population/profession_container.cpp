#include "metternich.h"

#include "population/profession_container.h"

#include "population/profession.h"

namespace metternich {

bool profession_compare::operator()(const profession *lhs, const profession *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
