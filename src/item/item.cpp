#include "metternich.h"

#include "item/item.h"

#include "item/item_type.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

item::item(const item_type *type)
	: type(type)
{
	assert_throw(this->get_type() != nullptr);

	this->update_name();
}

item::item(const gsml_data &scope)
{
	scope.process(this);
	this->update_name();
}

void item::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type") {
		this->type = item_type::get(value);
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
	data.add_property("equipped", string::from_bool(this->is_equipped()));

	return data;
}

void item::update_name()
{
	std::string name = this->get_type()->get_name();

	this->set_name(name);
}

const item_slot *item::get_slot() const
{
	return this->get_type()->get_slot();
}

const icon *item::get_icon() const
{
	return this->get_type()->get_icon();
}

}
