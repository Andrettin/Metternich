#include "metternich.h"

#include "item/enchantment.h"

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
}

}
