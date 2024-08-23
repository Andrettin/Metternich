#include "metternich.h"

#include "infrastructure/improvement_container.h"

#include "infrastructure/improvement.h"

namespace metternich {

bool improvement_compare::operator()(const improvement *lhs, const improvement *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
