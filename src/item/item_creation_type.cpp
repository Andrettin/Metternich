#include "metternich.h"

#include "item/item_creation_type.h"

#include "item/enchantment.h"
#include "item/item.h"
#include "item/item_type.h"
#include "script/condition/and_condition.h"
#include "spell/spell.h"
#include "util/assert_util.h"
#include "util/vector_random_util.h"

namespace metternich {

item_creation_type::item_creation_type(const std::string &identifier) : named_data_entry(identifier)
{
}

item_creation_type::~item_creation_type()
{
}

void item_creation_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "item_creation_subtypes") {
		for (const std::string &value : values) {
			this->item_creation_subtypes.push_back(item_creation_type::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->item_creation_subtypes.push_back(item_creation_type::get(property.get_key()));
			}
		});
	} else if (tag == "item_types") {
		for (const std::string &value : values) {
			this->item_types.push_back(item_type::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->item_types.push_back(item_type::get(property.get_key()));
			}
		});
	} else if (tag == "enchantments") {
		for (const std::string &value : values) {
			this->enchantments.push_back(enchantment::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->enchantments.push_back(enchantment::get(property.get_key()));
			}
		});
	} else if (tag == "spells") {
		for (const std::string &value : values) {
			this->spells.push_back(spell::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->spells.push_back(spell::get(property.get_key()));
			}
		});
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void item_creation_type::check() const
{
	if (!this->item_creation_subtypes.empty() && !this->item_types.empty()) {
		throw std::runtime_error(std::format("Item creation type \"{}\" has both item creation subtypes and information for creating items directly.", this->get_identifier()));
	}

	if (this->item_creation_subtypes.empty() && this->item_types.empty()) {
		throw std::runtime_error(std::format("Item creation type \"{}\" has neither item creation subtypes nor information for creating items directly.", this->get_identifier()));
	}
}

qunique_ptr<item> item_creation_type::create_item(const site *creation_site) const
{
	assert_throw(creation_site != nullptr);

	if (!this->item_creation_subtypes.empty()) {
		return vector::get_random(this->item_creation_subtypes)->create_item(creation_site);
	}

	assert_throw(!this->item_types.empty());
	const item_type *item_type = vector::get_random(this->item_types);

	const enchantment *enchantment = nullptr;
	if (!this->enchantments.empty()) {
		std::vector<const metternich::enchantment *> enchantments = this->enchantments;
		std::erase_if(enchantments, [this, creation_site](const metternich::enchantment *enchantment) {
			return enchantment->get_creation_site_conditions() != nullptr && !enchantment->get_creation_site_conditions()->check(creation_site, read_only_context(creation_site));
		});

		enchantment = vector::get_random(enchantments);
	}

	const spell *spell = nullptr;
	if (!this->spells.empty()) {
		spell = vector::get_random(this->spells);
	}

	return make_qunique<item>(item_type, nullptr, enchantment, spell);
}

}
