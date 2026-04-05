#include "metternich.h"

#include "item/recipe.h"

#include "character/trait.h"
#include "item/enchantment.h"
#include "item/item.h"
#include "item/item_creation_type.h"
#include "item/item_type.h"
#include "script/condition/and_condition.h"
#include "spell/spell.h"
#include "util/assert_util.h"

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

	if (tag == "required_traits") {
		for (const std::string &value : values) {
			this->required_traits.emplace_back(trait::get(value));
		}
	} else if (tag == "crafter_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->crafter_conditions = std::move(conditions);
	} else if (tag == "materials") {
		for (const std::string &value : values) {
			this->materials.emplace_back(item_type::get(value), nullptr);
		}

		scope.for_each_property([this](const gsml_property &property) {
			const item_type *item_type = item_type::get(property.get_key());
			const int quantity = std::stoi(property.get_value());
			this->materials.emplace_back(item_type, nullptr, quantity);
		});

		scope.for_each_child([this](const gsml_data &child_scope) {
			const item_type *item_type = item_type::get(child_scope.get_tag());
			const enchantment *enchantment = nullptr;
			int quantity = 1;

			child_scope.for_each_property([this, &enchantment, &quantity](const gsml_property &property) {
				const std::string &value = property.get_value();

				if (property.get_key() == "enchantment") {
					enchantment = enchantment::get(value);
				} else if (property.get_key() == "quantity") {
					quantity = std::stoi(value);
				} else {
					assert_throw(false);
				}
			});

			this->materials.emplace_back(item_type, enchantment, quantity);
		});
	} else if (tag == "spells") {
		for (const std::string &value : values) {
			this->spells.emplace_back(spell::get(value));
		}
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

void recipe::initialize()
{
	//add material components from spells to the materials of the recipe
	for (const spell *spell : this->get_spells()) {
		for (const item_type *material_component : spell->get_material_components()) {
			this->add_material(material_component, nullptr);
		}
	}

	named_data_entry::initialize();
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

void recipe::add_material(const item_type *item_type, const enchantment *enchantment)
{
	for (material &material : this->materials) {
		if (material.item_type == item_type && material.enchantment == enchantment) {
			++material.quantity;
			return;
		}
	}

	this->materials.emplace_back(item_type, enchantment);
}

int recipe::get_price() const
{
	return std::max(1, this->get_result_price() / 25);
}

int recipe::get_price_of_materials() const
{
	int price = 0;

	for (const material &material : this->get_materials()) {
		price += item::get_price(material.item_type, nullptr, material.enchantment, nullptr, nullptr) * material.quantity;
	}

	return price;
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
	return std::max(1, this->get_result_price() / 10 / 100);
}

std::string recipe::get_formula_string() const
{
	const int craft_cost = this->get_craft_cost();
	std::string str = std::format("{} Craft", craft_cost);

	for (const material &material : this->get_materials()) {
		str += std::format(" + {}x{}", material.quantity, item::create_name(material.item_type, nullptr, material.enchantment, nullptr, nullptr));
	}

	str += " \u2192 ";

	str += item::create_name(this->get_result_item_type(), nullptr, this->get_result_enchantment(), nullptr, nullptr);

	return str;
}

bool recipe::material::matches_item(const item *item) const
{
	if (item->get_type() != this->item_type) {
		return false;
	}

	if (item->get_material() != nullptr) {
		return false;
	}

	if (item->get_enchantment() != this->enchantment) {
		return false;
	}

	if (item->get_spell() != nullptr) {
		return false;
	}

	if (item->get_recipe() != nullptr) {
		return false;
	}

	return true;
}

}
