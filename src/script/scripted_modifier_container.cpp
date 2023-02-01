#include "metternich.h"

#include "script/scripted_modifier_container.h"

#include "script/scripted_modifier.h"

namespace metternich {

bool scripted_modifier_compare::operator()(const scripted_modifier *lhs, const scripted_modifier *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
