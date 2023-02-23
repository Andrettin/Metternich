#include "metternich.h"

#include "character/trait.h"

#include "character/attribute.h"
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
	} else if (tag == "generation_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		database::process_gsml_data(conditions, scope);
		this->generation_conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else if (tag == "military_unit_modifier") {
		auto modifier = std::make_unique<metternich::modifier<military_unit>>();
		database::process_gsml_data(modifier, scope);
		this->military_unit_modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void trait::check() const
{
	assert_throw(this->get_type() != trait_type::none);
	assert_throw(this->get_icon() != nullptr);

	if ((this->get_type() == trait_type::expertise || this->is_item()) && this->get_level() == 0) {
		throw std::runtime_error("Trait \"" + this->get_identifier() + "\" is an expertise or item trait, but has no level.");
	} else if (this->get_type() != trait_type::expertise && !this->is_item() && this->get_level() > 0) {
		throw std::runtime_error("Trait \"" + this->get_identifier() + "\" is not an expertise or item trait, but has a level.");
	}

	if (this->get_spell() != nullptr && !this->is_item()) {
		throw std::runtime_error("Trait \"" + this->get_identifier() + "\" is not an item trait, but grants a spell.");
	}
}

bool trait::is_item() const
{
	switch (this->get_type()) {
		case trait_type::weapon:
		case trait_type::armor:
		case trait_type::trinket:
			return true;
		default:
			return false;
	}
}

QString trait::get_modifier_string() const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_string());
}

QString trait::get_military_unit_modifier_string() const
{
	if (this->get_military_unit_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_military_unit_modifier()->get_string());
}

}
