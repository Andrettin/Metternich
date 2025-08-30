#include "metternich.h"

#include "character/character_class_container.h"

#include "character/character_class.h"

namespace metternich {

bool character_class_compare::operator()(const character_class *lhs, const character_class *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
