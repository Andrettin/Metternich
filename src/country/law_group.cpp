#include "metternich.h"

#include "country/law_group.h"

#include "util/container_util.h"

namespace metternich {

void law_group::check() const
{
	if (this->get_laws().empty()) {
		throw std::runtime_error(std::format("Law group \"{}\" has no laws.", this->get_identifier()));
	}
}

QVariantList law_group::get_laws_qvariant_list() const
{
	return container::to_qvariant_list(this->get_laws());
}

}
