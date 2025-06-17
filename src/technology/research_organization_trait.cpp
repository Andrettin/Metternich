#include "metternich.h"

#include "technology/research_organization_trait.h"

#include "country/idea_type.h"

namespace metternich {

research_organization_trait::research_organization_trait(const std::string &identifier)
	: idea_trait(identifier)
{
}

research_organization_trait::~research_organization_trait()
{
}

idea_type research_organization_trait::get_idea_type() const
{
	return idea_type::research_organization;
}

}
