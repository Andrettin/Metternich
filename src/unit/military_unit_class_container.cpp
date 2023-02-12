#include "metternich.h"

#include "unit/military_unit_class_container.h"

#include "unit/military_unit_class.h"

namespace metternich {

bool military_unit_class_compare::operator()(const military_unit_class *lhs, const military_unit_class *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	if (lhs->get_domain() != rhs->get_domain()) {
		return lhs->get_domain() < rhs->get_domain();
	}

	if (lhs->get_category() != rhs->get_category()) {
		return lhs->get_category() < rhs->get_category();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
