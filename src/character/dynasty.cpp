#include "metternich.h"

#include "character/dynasty.h"

namespace metternich {

void dynasty::check() const
{
	if (this->get_culture() == nullptr) {
		throw std::runtime_error(std::format("Dynasty \"{}\" has no culture.", this->get_identifier()));
	}
}

}
