#include "metternich.h"

#include "infrastructure/building_slot_type_container.h"

#include "infrastructure/building_slot_type.h"

namespace metternich {

bool building_slot_type_compare::operator()(const building_slot_type *lhs, const building_slot_type *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
