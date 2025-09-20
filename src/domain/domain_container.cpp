#include "metternich.h"

#include "domain/domain_container.h"

#include "domain/domain.h"

namespace metternich {

bool domain_compare::operator()(const domain *lhs, const domain *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
