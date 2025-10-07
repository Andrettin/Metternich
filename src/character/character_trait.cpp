#include "metternich.h"

#include "character/character_trait.h"

#include "character/character_attribute.h"
#include "character/trait_type.h"
#include "domain/office.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "util/assert_util.h"

namespace metternich {

character_trait::character_trait(const std::string &identifier)
	: trait_base(identifier)
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
			trait_type *trait_type = trait_type::get(value);
			trait_type->add_trait(this);
			this->types.push_back(trait_type);
		}
	} else if (tag == "attribute_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const character_attribute *attribute = character_attribute::get(property.get_key());
			const int value = std::stoi(property.get_value());

			this->attribute_bonuses[attribute] = value;
		});
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "generation_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->generation_conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else if (tag == "office_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const office *office = office::get(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const domain>>();
			modifier->process_gsml_data(child_scope);
			this->office_modifiers[office] = std::move(modifier);
		});
	} else if (tag == "scaled_office_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const office *office = office::get(child_scope.get_tag());
			auto modifier = std::make_unique<metternich::modifier<const domain>>();
			modifier->process_gsml_data(child_scope);
			this->scaled_office_modifiers[office] = std::move(modifier);
		});
	} else if (tag == "military_unit_modifier") {
		auto modifier = std::make_unique<metternich::modifier<military_unit>>();
		modifier->process_gsml_data(scope);
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

	if (!this->scaled_office_modifiers.empty() && this->get_attribute() == nullptr) {
		throw std::runtime_error(std::format("Character trait \"{}\" with scaled office modifier has no attribute.", this->get_identifier()));
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
