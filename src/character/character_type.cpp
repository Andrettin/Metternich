#include "metternich.h"

#include "character/character_type.h"

#include "character/attribute.h"
#include "util/assert_util.h"

namespace metternich {

character_type::character_type(const std::string &identifier)
	: named_data_entry(identifier), primary_attribute(attribute::none)
{
}

void character_type::check() const
{
	assert_throw(this->get_portrait() != nullptr);
	assert_throw(this->get_primary_attribute() != attribute::none);
}

}
