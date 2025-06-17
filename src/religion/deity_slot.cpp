#include "metternich.h"

#include "religion/deity_slot.h"

#include "country/idea_type.h"

namespace metternich {

deity_slot::deity_slot(const std::string &identifier)
	: idea_slot(identifier)
{
}

deity_slot::~deity_slot() = default;

idea_type deity_slot::get_idea_type() const
{
	return idea_type::deity;
}

}
