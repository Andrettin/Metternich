#include "metternich.h"

#include "technology/research_organization.h"

#include "technology/research_organization_trait.h"

namespace metternich {
	
void research_organization::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Research organization \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_traits().empty()) {
		throw std::runtime_error(std::format("Research organization \"{}\" has no traits.", this->get_identifier()));
	}

	if (this->get_skill() == 0) {
		for (const research_organization_trait *trait : this->get_traits()) {
			if (trait->get_scaled_modifier() != nullptr) {
				throw std::runtime_error(std::format("Research organization \"{}\" has a trait with a scaled modifier, but no skill value.", this->get_identifier()));
			}
		}
	}
}

}
