#include "metternich.h"

#include "spell/arcane_school.h"

#include "util/log_util.h"

namespace metternich {

void arcane_school::check() const
{
	if (this->get_spells().empty()) {
		log::log_error(std::format("Arcane school \"{}\" has no spells.", this->get_identifier()));
	}

	log_trace(std::format("Arcane school \"{}\" has {} spells.", this->get_identifier(), this->get_spells().size()));
}

}
