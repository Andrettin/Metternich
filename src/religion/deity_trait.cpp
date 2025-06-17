#include "metternich.h"

#include "religion/deity_trait.h"

#include "country/idea_type.h"

namespace metternich {

deity_trait::deity_trait(const std::string &identifier)
	: idea_trait(identifier)
{
}

deity_trait::~deity_trait()
{
}

idea_type deity_trait::get_idea_type() const
{
	return idea_type::deity;
}

}
