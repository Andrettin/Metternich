#include "metternich.h"

#include "item/item.h"

#include "item/affix_type.h"
#include "item/enchantment.h"
#include "item/item_material.h"
#include "item/item_type.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

item::item(const item_type *type, const item_material *material, const metternich::enchantment *enchantment)
	: type(type), material(material), enchantment(enchantment)
{
	assert_throw(this->get_type() != nullptr);

	if (this->get_enchantment() != nullptr) {
		assert_throw(this->get_enchantment()->is_allowed_for_item_type(this->get_type()));
	}

	if (this->get_material() != nullptr) {
		assert_throw(this->get_material()->is_allowed_for_item_type(this->get_type()));
	}

	this->update_name();
}

item::item(const gsml_data &scope)
{
	scope.process(this);

	assert_throw(this->get_type() != nullptr);

	this->update_name();
}

void item::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type") {
		this->type = item_type::get(value);
	} else if (key == "material") {
		this->material = item_material::get(value);
	} else if (key == "enchantment") {
		this->enchantment = enchantment::get(value);
	} else if (key == "equipped") {
		this->equipped = string::to_bool(value);
	} else {
		throw std::runtime_error(std::format("Invalid item property: \"{}\".", key));
	}
}

void item::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	throw std::runtime_error(std::format("Invalid item scope: \"{}\".", tag));
}

gsml_data item::to_gsml_data() const
{
	gsml_data data;

	data.add_property("type", this->get_type()->get_identifier());

	if (this->get_material() != nullptr) {
		data.add_property("material", this->get_material()->get_identifier());
	}

	if (this->get_enchantment() != nullptr) {
		data.add_property("enchantment", this->get_enchantment()->get_identifier());
	}

	data.add_property("equipped", string::from_bool(this->is_equipped()));

	return data;
}

std::string item::create_name(const item_type *type, const item_material *material, const metternich::enchantment *enchantment)
{
	std::string name = type->get_name();

	if (material != nullptr) {
		name = material->get_name() + " " + name;
	}

	if (enchantment != nullptr) {
		switch (enchantment->get_affix_type()) {
			case affix_type::prefix:
				name = enchantment->get_name() + " " + name;
				break;
			case affix_type::suffix:
				name += " " + enchantment->get_name();
				break;
			case affix_type::stem:
				name = enchantment->get_name();
				break;
		}
	}

	return name;
}

void item::update_name()
{
	this->set_name(item::create_name(this->get_type(), this->get_material(), this->get_enchantment()));
}

const item_slot *item::get_slot() const
{
	return this->get_type()->get_slot();
}

const icon *item::get_icon() const
{
	return this->get_type()->get_icon();
}

void item::change_quantity(const int change)
{
	assert_throw(this->get_type()->is_stackable());

	this->quantity += change;
	
	assert_throw(this->quantity >= 0);
}

}
