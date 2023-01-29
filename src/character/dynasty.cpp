#include "metternich.h"

#include "character/dynasty.h"

#include "util/assert_util.h"

namespace metternich {

void dynasty::check() const
{
	if (this->get_culture() == nullptr) {
		throw std::runtime_error("Dynasty \"" + this->get_identifier() + "\" has no culture.");
	}
}

}
