#include "metternich.h"

#include "unit/transporter_class_container.h"

#include "unit/transporter_class.h"

namespace metternich {

bool transporter_class_compare::operator()(const transporter_class *lhs, const transporter_class *rhs) const
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
