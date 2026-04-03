#include "metternich.h"

#include "item/recipe.h"

#include "item/enchantment.h"
#include "item/item_creation_type.h"
#include "item/item_type.h"
#include "script/condition/and_condition.h"

namespace metternich {

recipe::recipe(const std::string &identifier) : named_data_entry(identifier)
{
}

recipe::~recipe()
{
}

void recipe::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "crafter_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->crafter_conditions = std::move(conditions);
	} else if (tag == "item_creation_types") {
		for (const std::string &value : values) {
			item_creation_type *item_creation_type = item_creation_type::get(value);
			item_creation_type->add_recipe(this);
		}

		scope.for_each_property([this](const gsml_property &property) {
			item_creation_type *item_creation_type = item_creation_type::get(property.get_key());
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				item_creation_type->add_recipe(this);
			}
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void recipe::check() const
{
	if (this->get_result_item_type() == nullptr) {
		throw std::runtime_error(std::format("Recipe \"{}\" has no result item type.", this->get_identifier()));
	}
}

const icon *recipe::get_icon() const
{
	return this->get_result_item_type()->get_icon();
}

int recipe::get_price() const
{
	return std::max(1, this->get_result_price() / 2);
}

int recipe::get_result_price() const
{
	int price = this->get_result_item_type()->get_price();

	if (this->get_result_enchantment() != nullptr) {
		price += this->get_result_enchantment()->get_price();
	}

	return price;
}

int recipe::get_craft_cost() const
{
	return std::max(1, this->get_result_price() / 10);
}

}
