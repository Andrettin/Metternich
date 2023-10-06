#include "metternich.h"

#include "country/government_type_container.h"

#include "country/government_type.h"

namespace metternich {

bool government_type_compare::operator()(const government_type *lhs, const government_type *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
