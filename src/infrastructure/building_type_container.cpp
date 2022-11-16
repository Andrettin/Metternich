#include "metternich.h"

#include "infrastructure/building_type_container.h"

#include "infrastructure/building_type.h"

namespace metternich {

bool building_type_compare::operator()(const building_type *lhs, const building_type *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
