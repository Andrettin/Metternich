#include "metternich.h"

#include "character/monster_type.h"

#include "character/character_class.h"
#include "species/species.h"

namespace metternich {

monster_type::monster_type(const std::string &identifier)
	: named_data_entry(identifier)
{
}

monster_type::~monster_type()
{
}

void monster_type::check() const
{
	if (this->get_species() == nullptr) {
		throw std::runtime_error(std::format("Monster type \"{}\" has no species.", this->get_identifier()));
	}
}

}
