#include "metternich.h"

#include "country/law_group.h"

#include "country/law.h"
#include "util/container_util.h"

namespace metternich {

void law_group::check() const
{
	if (this->get_laws().empty()) {
		throw std::runtime_error(std::format("Law group \"{}\" has no laws.", this->get_identifier()));
	}

	if (this->get_default_law() == nullptr) {
		throw std::runtime_error(std::format("Law group \"{}\" has no default law.", this->get_identifier()));
	}

	if (this->get_default_law()->get_group() != this) {
		throw std::runtime_error(std::format("The default law for law group \"{}\" does not belong to the group.", this->get_identifier()));
	}
}

QVariantList law_group::get_laws_qvariant_list() const
{
	return container::to_qvariant_list(this->get_laws());
}

}
