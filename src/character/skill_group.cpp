#include "metternich.h"

#include "character/skill_group.h"

namespace metternich {

void skill_group::check() const
{
	if (this->get_skills().empty()) {
		throw std::runtime_error(std::format("Skill group \"{}\" has no skills.", this->get_identifier()));
	}
}

}
