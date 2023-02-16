#include "metternich.h"

#include "unit/military_unit_type_container.h"

#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"

namespace metternich {

bool military_unit_type_compare::operator()(const military_unit_type *lhs, const military_unit_type *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	const military_unit_class *lhs_class = lhs->get_unit_class();
	const military_unit_class *rhs_class = rhs->get_unit_class();

	if (lhs_class != rhs_class) {
		if (lhs_class->get_domain() != rhs_class->get_domain()) {
			return lhs_class->get_domain() < rhs_class->get_domain();
		}

		if (lhs_class->get_category() != rhs_class->get_category()) {
			return lhs_class->get_category() < rhs_class->get_category();
		}
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
