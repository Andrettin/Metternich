#include "metternich.h"

#include "technology/research_organization.h"

namespace metternich {
	
void research_organization::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Research organization \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_traits().empty()) {
		throw std::runtime_error(std::format("Research organization \"{}\" has no traits.", this->get_identifier()));
	}
}

}
