#include "metternich.h"

#include "character/trait.h"

#include "character/attribute.h"
#include "character/character_type.h"
#include "character/trait_type.h"
#include "script/modifier.h"
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
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const character>>();
		database::process_gsml_data(this->modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void trait::check() const
{
	assert_throw(this->get_type() != trait_type::none);
	assert_throw(this->get_icon() != nullptr);

	if (this->get_type() == trait_type::expertise && this->get_level() == 0) {
		throw std::runtime_error("Trait \"" + this->get_identifier() + "\" is an expertise trait, but has no level.");
	} else if (this->get_type() != trait_type::expertise && this->get_level() > 0) {
		throw std::runtime_error("Trait \"" + this->get_identifier() + "\" is not an expertise trait, but has a level.");
	}
}

QString trait::get_modifier_string() const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_string());
}

}
