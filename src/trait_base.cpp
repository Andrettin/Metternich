#include "metternich.h"

#include "trait_base.h"

namespace metternich {

trait_base::trait_base(const std::string &identifier)
	: named_data_entry(identifier)
{
}

void trait_base::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Trait \"{}\" has no icon.", this->get_identifier()));
	}
}

}
