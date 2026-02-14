#include "metternich.h"

#include "culture/culture_container.h"

#include "culture/culture.h"

namespace metternich {

bool culture_compare::operator()(const culture *lhs, const culture *rhs) const
{
	if (lhs == nullptr || rhs == nullptr) {
		return lhs == nullptr && rhs != nullptr;
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
