#include "metternich.h"

#include "infrastructure/pathway_container.h"

#include "infrastructure/pathway.h"

namespace metternich {

bool pathway_compare::operator()(const pathway *lhs, const pathway *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	if (lhs->get_transport_level() != rhs->get_transport_level()) {
		return lhs->get_transport_level() < rhs->get_transport_level();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
