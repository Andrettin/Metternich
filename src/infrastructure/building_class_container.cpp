#include "metternich.h"

#include "infrastructure/building_class_container.h"

#include "infrastructure/building_class.h"

namespace metternich {

bool building_class_compare::operator()(const building_class *lhs, const building_class *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
