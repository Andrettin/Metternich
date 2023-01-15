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

QString character_type::get_primary_attribute_name_qstring() const
{
	return QString::fromStdString(get_attribute_name(this->get_primary_attribute()));
}

}
