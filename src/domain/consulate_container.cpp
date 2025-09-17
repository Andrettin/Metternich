#include "metternich.h"

#include "domain/consulate_container.h"

#include "domain/consulate.h"

namespace metternich {

bool consulate_compare::operator()(const consulate *lhs, const consulate *rhs) const
{
	if (lhs->get_level() != rhs->get_level()) {
		return lhs->get_level() < rhs->get_level();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
