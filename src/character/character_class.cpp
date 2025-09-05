#include "metternich.h"

#include "character/character_class.h"

#include "character/character_attribute.h"
#include "character/starting_age_category.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "ui/portrait.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"

namespace metternich {

character_class::character_class(const std::string &identifier)
	: named_data_entry(identifier), attribute(character_attribute::none), military_unit_category(military_unit_category::none)
{
}

character_class::~character_class()
{
}

void character_class::check() const
{
	if (this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Character type \"{}\" has no attribute.", this->get_identifier()));
	}

	if (this->get_starting_age_category() == starting_age_category::none) {
		throw std::runtime_error(std::format("Character type \"{}\" has no starting age category.", this->get_identifier()));
	}
}

}
