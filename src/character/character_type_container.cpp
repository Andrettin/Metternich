#include "metternich.h"

#include "character/character_type_container.h"

#include "character/character_type.h"

namespace metternich {

bool character_type_compare::operator()(const character_type *lhs, const character_type *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
