#include "metternich.h"

#include "technology/research_organization_slot.h"

#include "country/idea_type.h"

namespace metternich {

research_organization_slot::research_organization_slot(const std::string &identifier)
	: idea_slot(identifier)
{
}

research_organization_slot::~research_organization_slot() = default;

idea_type research_organization_slot::get_idea_type() const
{
	return idea_type::research_organization;
}

}
