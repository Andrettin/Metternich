#include "metternich.h"

#include "character/trait.h"

#include "character/character_attribute.h"
#include "character/trait_type.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"

namespace metternich {

trait::trait(const std::string &identifier)
	: named_data_entry(identifier), type(trait_type::none), attribute(character_attribute::none)
{
}

trait::~trait()
{
}

void trait::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "attribute_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const character_attribute attribute = enum_converter<character_attribute>::to_enum(property.get_key());
			const int value = std::stoi(property.get_value());

			this->attribute_bonuses[attribute] = value;
		});
	} else if (tag == "conditions") {
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
	} else if (tag == "ruler_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->ruler_modifier = std::move(modifier);
	} else if (tag == "scaled_ruler_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->scaled_ruler_modifier = std::move(modifier);
	} else if (tag == "advisor_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->advisor_modifier = std::move(modifier);
	} else if (tag == "scaled_advisor_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->scaled_advisor_modifier = std::move(modifier);
	} else if (tag == "governor_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const province>>();
		database::process_gsml_data(modifier, scope);
		this->governor_modifier = std::move(modifier);
	} else if (tag == "scaled_governor_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const province>>();
		database::process_gsml_data(modifier, scope);
		this->scaled_governor_modifier = std::move(modifier);
	} else if (tag == "leader_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		database::process_gsml_data(modifier, scope);
		this->leader_modifier = std::move(modifier);
	} else if (tag == "scaled_leader_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		database::process_gsml_data(modifier, scope);
		this->scaled_leader_modifier = std::move(modifier);
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
	if (this->get_type() == trait_type::none) {
		throw std::runtime_error(std::format("Trait \"{}\" has no type.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Trait \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_scaled_ruler_modifier() != nullptr && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Trait \"{}\" with scaled ruler modifier has no attribute.", this->get_identifier()));
	}

	if (this->get_scaled_advisor_modifier() != nullptr && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Trait \"{}\" with scaled advisor modifier has no attribute.", this->get_identifier()));
	}

	if (this->get_scaled_governor_modifier() != nullptr && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Trait \"{}\" with scaled governor modifier has no attribute.", this->get_identifier()));
	}

	if (this->get_scaled_leader_modifier() != nullptr && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Trait \"{}\" with scaled leader modifier has no attribute.", this->get_identifier()));
	}
}

QString trait::get_modifier_string() const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_string(nullptr));
}

QString trait::get_military_unit_modifier_string() const
{
	if (this->get_military_unit_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_military_unit_modifier()->get_string(nullptr));
}

}
