#include "metternich.h"

#include "character/character_type.h"

#include "character/attribute.h"
#include "util/assert_util.h"

namespace metternich {

character_type::character_type(const std::string &identifier)
	: named_data_entry(identifier), primary_attribute(attribute::none)
{
}

void character_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
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
