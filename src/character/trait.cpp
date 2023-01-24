#include "metternich.h"

#include "character/trait.h"

#include "character/attribute.h"
#include "character/character_type.h"
#include "character/trait_type.h"
#include "script/condition/and_condition.h"
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

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const character>>();
		database::process_gsml_data(this->modifier, scope);
	} else if (tag == "character_type_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const character_type *character_type = character_type::get(child_tag);

			auto modifier = std::make_unique<metternich::modifier<const character>>();
			database::process_gsml_data(modifier, child_scope);

			this->character_type_modifiers[character_type] = std::move(modifier);
		});
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

QString trait::get_modifier_string(metternich::character_type *character_type) const
{
	QString str = this->get_modifier_string();

	const metternich::modifier<const character> *character_type_modifier = this->get_character_type_modifier(character_type);
	if (character_type_modifier != nullptr) {
		if (!str.isEmpty()) {
			str += "\n";
		}

		str += QString::fromStdString(character_type_modifier->get_string());
	}

	return str;
}

}
