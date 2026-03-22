#include "metternich.h"

#include "item/item_type.h"

#include "database/defines.h"
#include "economy/commodity.h"
#include "item/item_class.h"
#include "item/item_slot.h"
#include "script/modifier.h"

namespace metternich {

item_type::item_type(const std::string &identifier) : named_data_entry(identifier)
{
}

item_type::~item_type()
{
}

void item_type::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "price") {
		this->price = defines::get()->get_wealth_commodity()->string_to_value(value);
	} else {
		data_entry::process_gsml_property(property);
	}
}

void item_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void item_type::initialize()
{
	this->damage_dice.set_min_value(0);

	named_data_entry::initialize();
}

void item_type::check() const
{
	if (this->get_item_class() == nullptr) {
		throw std::runtime_error(std::format("Item type \"{}\" has no item class.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Item type \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_price() == 0) {
		throw std::runtime_error(std::format("Item type \"{}\" has no price.", this->get_identifier()));
	}

	if (this->is_weapon() && this->get_damage_dice().is_null()) {
		throw std::runtime_error(std::format("Item type \"{}\" is a weapon, but has no damage dice.", this->get_identifier()));
	}
}

const item_slot *item_type::get_slot() const
{
	return this->get_item_class()->get_slot();
}

bool item_type::is_weapon() const
{
	return this->get_item_class()->is_weapon();
}

}
