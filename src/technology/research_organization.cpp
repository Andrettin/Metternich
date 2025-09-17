#include "metternich.h"

#include "technology/research_organization.h"

#include "domain/idea_type.h"
#include "technology/research_organization_trait.h"
#include "technology/technology.h"

namespace metternich {

research_organization::research_organization(const std::string &identifier) : idea(identifier)
{
}

research_organization::~research_organization() = default;

void research_organization::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "traits") {
		for (const std::string &value : values) {
			this->add_trait(research_organization_trait::get(value));
		}
	} else {
		idea::process_gsml_scope(scope);
	}
}

void research_organization::initialize()
{
	if (this->get_required_technology() != nullptr) {
		this->get_required_technology()->add_enabled_research_organization(this);
	}

	if (this->get_obsolescence_technology() != nullptr) {
		this->get_obsolescence_technology()->add_disabled_research_organization(this);
	}

	idea::initialize();
}

idea_type research_organization::get_idea_type() const
{
	return idea_type::research_organization;
}

}
