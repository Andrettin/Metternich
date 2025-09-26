#include "metternich.h"

#include "character/saving_throw_type.h"

namespace metternich {

void saving_throw_type::check() const
{
	if (this->get_attribute() == nullptr) {
		//throw std::runtime_error(std::format("Saving throw type \"{}\" has no attribute.", this->get_identifier()));
	}
}

}
