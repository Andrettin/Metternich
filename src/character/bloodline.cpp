#include "metternich.h"

#include "character/bloodline.h"

#include "character/character.h"

namespace metternich {

void bloodline::check() const
{
	if (this->get_founder() == nullptr) {
		throw std::runtime_error(std::format("Bloodline \"{}\" has no founder.", this->get_identifier()));
	}
}

void bloodline::set_founder(character *founder)
{
	if (founder == this->get_founder()) {
		return;
	}

	this->founder = founder;
	founder->set_bloodline(this);
	founder->set_bloodline_strength(bloodline::max_bloodline_strength);
}

}
