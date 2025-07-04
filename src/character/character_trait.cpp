#include "metternich.h"

#include "character/character_trait.h"

#include "character/character_attribute.h"
#include "character/character_trait_type.h"
#include "country/office.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "util/assert_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

character_trait::character_trait(const std::string &identifier)
	: trait_base(identifier), attribute(character_attribute::none)
{
}

character_trait::~character_trait()
{
}

void character_trait::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "types") {
		for (const std::string &value : values) {
			this->types.insert(magic_enum::enum_cast<character_trait_type>(value).value());
		}
	} else if (tag == "attribute_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const character_attribute attribute = magic_enum::enum_cast<character_attribute>(property.get_key()).value();
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
	} else if (tag == "office_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const office *office = office::get(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const country>>();
			database::process_gsml_data(modifier, child_scope);
			this->office_modifiers[office] = std::move(modifier);
		});
	} else if (tag == "scaled_office_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const office *office = office::get(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const country>>();
			database::process_gsml_data(modifier, child_scope);
			this->scaled_office_modifiers[office] = std::move(modifier);
		});
	} else if (tag == "advisor_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->advisor_modifier = std::move(modifier);
	} else if (tag == "scaled_advisor_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->scaled_advisor_modifier = std::move(modifier);
	} else if (tag == "advisor_effects") {
		auto effect_list = std::make_unique<metternich::effect_list<const country>>();
		database::process_gsml_data(effect_list, scope);
		this->advisor_effects = std::move(effect_list);
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
		trait_base::process_gsml_scope(scope);
	}
}

void character_trait::check() const
{
	if (this->get_types().empty()) {
		throw std::runtime_error(std::format("Character trait \"{}\" has no type.", this->get_identifier()));
	}

	if (!this->scaled_office_modifiers.empty() && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Character trait \"{}\" with scaled office modifier has no attribute.", this->get_identifier()));
	}

	if (this->get_scaled_advisor_modifier() != nullptr && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Character trait \"{}\" with scaled advisor modifier has no attribute.", this->get_identifier()));
	}

	if (this->get_advisor_effects() != nullptr) {
		this->get_advisor_effects()->check();
	}

	if (this->get_scaled_governor_modifier() != nullptr && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Character trait \"{}\" with scaled governor modifier has no attribute.", this->get_identifier()));
	}

	if (this->get_scaled_leader_modifier() != nullptr && this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Character trait \"{}\" with scaled leader modifier has no attribute.", this->get_identifier()));
	}

	trait_base::check();
}

QString character_trait::get_modifier_string() const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_string(nullptr));
}

QString character_trait::get_military_unit_modifier_string() const
{
	if (this->get_military_unit_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_military_unit_modifier()->get_string(nullptr));
}

}
