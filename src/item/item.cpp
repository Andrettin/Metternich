#include "metternich.h"

#include "item/item.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/defines.h"
#include "item/affix_type.h"
#include "item/enchantment.h"
#include "item/item_class.h"
#include "item/item_material.h"
#include "item/item_type.h"
#include "item/recipe.h"
#include "script/modifier.h"
#include "spell/spell.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace metternich {

QVariantMap item_key::to_qvariant_map() const
{
	QVariantMap qvariant_map;

	qvariant_map["type"] = QVariant::fromValue(this->type);
	qvariant_map["material"] = QVariant::fromValue(this->material);
	qvariant_map["enchantment"] = QVariant::fromValue(this->enchantment);
	qvariant_map["spell"] = QVariant::fromValue(this->spell);
	qvariant_map["recipe"] = QVariant::fromValue(this->recipe);

	return qvariant_map;
}

bool item_key::operator <(const item_key &other) const
{
	if (this->type != other.type) {
		return this->type->get_identifier() < other.type->get_identifier();
	}

	if (this->material != other.material) {
		if (this->material != nullptr && other.material != nullptr) {
			return this->material->get_identifier() < other.material->get_identifier();
		} else {
			return this->material == nullptr;
		}
	}

	if (this->enchantment != other.enchantment) {
		if (this->enchantment != nullptr && other.enchantment != nullptr) {
			return this->enchantment->get_identifier() < other.enchantment->get_identifier();
		} else {
			return this->enchantment == nullptr;
		}
	}

	if (this->spell != other.spell) {
		if (this->spell != nullptr && other.spell != nullptr) {
			return this->spell->get_identifier() < other.spell->get_identifier();
		} else {
			return this->spell == nullptr;
		}
	}

	if (this->recipe != other.recipe) {
		if (this->recipe != nullptr && other.recipe != nullptr) {
			return this->recipe->get_identifier() < other.recipe->get_identifier();
		} else {
			return this->recipe == nullptr;
		}
	}

	return false;
}

item::item(const item_type *type, const item_material *material, const metternich::enchantment *enchantment, const metternich::spell *spell, const metternich::recipe *recipe)
	: type(type), material(material), enchantment(enchantment), spell(spell), recipe(recipe)
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

item::~item()
{
}

void item::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "name") {
		this->name = value;
	} else if (key == "type") {
		this->type = item_type::get(value);
	} else if (key == "material") {
		this->material = item_material::get(value);
	} else if (key == "enchantment") {
		this->enchantment = enchantment::get(value);
	} else if (key == "spell") {
		this->spell = spell::get(value);
	} else if (key == "recipe") {
		this->recipe = recipe::get(value);
	} else if (key == "equipped") {
		this->equipped = string::to_bool(value);
	} else if (key == "quantity") {
		this->quantity = std::stoi(value);
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

	data.add_property("name", std::format("\"{}\"", this->get_name()));
	data.add_property("type", this->get_type()->get_identifier());

	if (this->get_material() != nullptr) {
		data.add_property("material", this->get_material()->get_identifier());
	}

	if (this->get_enchantment() != nullptr) {
		data.add_property("enchantment", this->get_enchantment()->get_identifier());
	}

	if (this->get_spell() != nullptr) {
		data.add_property("spell", this->get_spell()->get_identifier());
	}

	if (this->get_recipe() != nullptr) {
		data.add_property("recipe", this->get_recipe()->get_identifier());
	}

	data.add_property("equipped", string::from_bool(this->is_equipped()));
	data.add_property("quantity", std::to_string(this->get_quantity()));

	return data;
}

std::string item::create_name(const item_type *type, const item_material *material, const metternich::enchantment *enchantment, const metternich::spell *spell, const metternich::recipe *recipe)
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

	if (spell != nullptr) {
		name += " of " + spell->get_name();
	}

	if (recipe != nullptr) {
		name = "Recipe of " + recipe->get_name();
	}

	if (enchantment != nullptr) {
		return string::colored(name, defines::get()->get_magic_item_text_color());
	} else {
		return name;
	}
}

int item::get_price(const item_type *type, const item_material *material, const metternich::enchantment *enchantment, const metternich::spell *spell, const metternich::recipe *recipe)
{
	Q_UNUSED(material);

	int price = type->get_price();

	if (enchantment != nullptr) {
		price += enchantment->get_price();
	}

	if (spell != nullptr) {
		price += spell->get_price();
	}

	if (recipe != nullptr) {
		price += recipe->get_price();
	}

	return price;
}

void item::update_name()
{
	this->set_name(item::create_name(this->get_type(), this->get_material(), this->get_enchantment(), this->get_spell(), this->get_recipe()));
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

	emit quantity_changed();
}

QString item::get_effects_string(const character *character) const
{
	std::string str;

	if (this->get_type()->get_modifier() != nullptr) {
		str += this->get_type()->get_modifier()->get_single_line_string(nullptr);
	}

	if (this->get_material() != nullptr && this->get_material()->get_modifier() != nullptr) {
		if (!str.empty()) {
			str += ", ";
		}

		str += this->get_material()->get_modifier()->get_single_line_string(nullptr);
	}

	if (this->get_enchantment() != nullptr) {
		if (!str.empty()) {
			str += ", ";
		}

		str += this->get_enchantment()->get_effects_string();
	}

	if (this->get_spell() != nullptr) {
		if (!str.empty()) {
			str += ", ";
		}

		str += std::format("{} Spell", this->get_spell()->get_name());
	}

	if (this->get_recipe() != nullptr) {
		if (!str.empty()) {
			str += ", ";
		}

		str += std::format("{} Recipe", this->get_recipe()->get_name());
	}

	std::string reason;
	if ((this->get_slot() != nullptr || this->get_type()->get_item_class()->is_consumable()) && !character->get_game_data()->can_use_item(this, &reason)) {
		if (reason.empty()) {
			reason = std::format("cannot {}", this->get_slot() != nullptr ? "equip" : this->get_type()->get_item_class()->get_consume_verb());
		}
		str += " " + string::colored(std::format("({})", reason), defines::get()->get_red_text_color());
	}

	return QString::fromStdString(str);
}

int item::get_price() const
{
	return item::get_price(this->get_type(), this->get_material(), this->get_enchantment(), this->get_spell(), this->get_recipe());
}

bool item::is_useful_for(const character *character) const
{
	if (this->get_slot() != nullptr) {
		if (!character->get_game_data()->can_equip_item(this, false, this->get_price())) {
			return false;
		}

		return true;
	} else if (this->get_type()->get_item_class()->is_consumable()) {
		if (!character->get_game_data()->can_consume_item(this)) {
			return false;
		}

		return true;
	}

	return false;
}

}
