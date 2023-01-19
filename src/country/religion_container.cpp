#include "metternich.h"

#include "country/religion_container.h"

#include "country/religion.h"

namespace metternich {

bool religion_compare::operator()(const religion *lhs, const religion *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
