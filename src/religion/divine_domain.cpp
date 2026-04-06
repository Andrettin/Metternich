#include "metternich.h"

#include "religion/divine_domain.h"

#include "util/log_util.h"

namespace metternich {

void divine_domain::check() const
{
	if (this->get_spells().empty()) {
		log::log_error(std::format("Divine domain \"{}\" has no spells.", this->get_identifier()));
	}

	log_trace(std::format("Divine domain \"{}\" has {} spells.", this->get_identifier(), this->get_spells().size()));
}

}
