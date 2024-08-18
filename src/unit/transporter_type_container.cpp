#include "metternich.h"

#include "unit/transporter_type_container.h"

#include "unit/transporter_type.h"

namespace metternich {

bool transporter_type_compare::operator()(const transporter_type *lhs, const transporter_type *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	if (lhs->get_category() != rhs->get_category()) {
		return lhs->get_category() < rhs->get_category();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
