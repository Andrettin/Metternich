#include "metternich.h"

#include "unit/civilian_unit_class_container.h"

#include "unit/civilian_unit_class.h"

namespace metternich {

bool civilian_unit_class_compare::operator()(const civilian_unit_class *lhs, const civilian_unit_class *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
