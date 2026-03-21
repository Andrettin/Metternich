#include "metternich.h"

#include "spell/spell_container.h"

#include "spell/spell.h"

namespace metternich {

bool spell_compare::operator()(const spell *lhs, const spell *rhs) const
{
	if (lhs->get_level() != rhs->get_level()) {
		return lhs->get_level() < rhs->get_level();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
