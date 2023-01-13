#include "metternich.h"

#include "character/trait.h"

#include "character/attribute.h"
#include "character/character_type.h"
#include "character/trait_type.h"
#include "util/assert_util.h"

namespace metternich {

trait::trait(const std::string &identifier)
	: named_data_entry(identifier), type(trait_type::none)
{
}

void trait::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "character_types") {
		for (const std::string &value : values) {
			this->character_types.insert(character_type::get(value));
		}
	} else if (tag == "attribute_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->attribute_bonuses[enum_converter<attribute>::to_enum(key)] = std::stoi(value);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void trait::check() const
{
	assert_throw(this->get_type() != trait_type::none);
	assert_throw(this->get_icon() != nullptr);
}

}
