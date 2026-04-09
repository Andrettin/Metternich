#include "metternich.h"

#include "character/domain_skill.h"

namespace metternich {

void domain_skill::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Domain skill \"{}\" has no icon.", this->get_identifier()));
	}

	character_stat::check();
}

const metternich::icon *domain_skill::get_icon() const
{
	return this->icon;
}

}
