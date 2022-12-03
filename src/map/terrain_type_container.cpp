#include "metternich.h"

#include "map/terrain_type_container.h"

#include "map/terrain_type.h"

namespace metternich {

bool terrain_type_compare::operator()(const terrain_type *lhs, const terrain_type *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
