#include "metternich.h"

#include "item/enchantment.h"

#include "database/defines.h"
#include "economy/commodity.h"
#include "item/affix_type.h"
#include "item/item_class.h"
#include "item/item_type.h"
#include "script/modifier.h"

namespace metternich {

enchantment::enchantment(const std::string &identifier) : named_data_entry(identifier)
{
}

enchantment::~enchantment()
{
}

void enchantment::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "price") {
		this->price = defines::get()->get_wealth_commodity()->string_to_value(value);
	} else {
		data_entry::process_gsml_property(property);
	}
}

void enchantment::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "item_classes") {
		for (const std::string &value : values) {
			this->item_classes.insert(item_class::get(value));
		}
	} else if (tag == "item_types") {
		for (const std::string &value : values) {
			this->item_types.insert(item_type::get(value));
		}
	} else if (tag == "subenchantments") {
		for (const std::string &value : values) {
			this->subenchantments.push_back(enchantment::get(value));
		}
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void enchantment::check() const
{
	if (this->get_affix_type() == affix_type::none) {
		throw std::runtime_error(std::format("Enchantment \"{}\" has no affix type.", this->get_identifier()));
	}

	if (this->get_item_classes().empty() && this->get_item_types().empty()) {
		throw std::runtime_error(std::format("Enchantment \"{}\" has neither item classes nor item types.", this->get_identifier()));
	}

	if (this->get_price() == 0) {
		throw std::runtime_error(std::format("Enchantment \"{}\" has no price.", this->get_identifier()));
	}
}

bool enchantment::is_allowed_for_item_type(const item_type *item_type) const
{
	return this->get_item_types().contains(item_type) || (item_type->get_item_class() != nullptr && this->get_item_classes().contains(item_type->get_item_class()));
}

std::string enchantment::get_effects_string() const
{
	std::string str;

	if (this->get_modifier() != nullptr) {
		str += this->get_modifier()->get_single_line_string(nullptr);
	}

	for (const enchantment *subenchantment : this->get_subenchantments()) {
		if (!str.empty()) {
			str += ", ";
		}

		str += subenchantment->get_effects_string();
	}

	return str;
}

}
