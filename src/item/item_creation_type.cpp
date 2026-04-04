#include "metternich.h"

#include "item/item_creation_type.h"

#include "item/enchantment.h"
#include "item/item.h"
#include "item/item_type.h"
#include "item/recipe.h"
#include "script/condition/and_condition.h"
#include "spell/spell.h"
#include "util/assert_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

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
			const item_type *item_type = item_type::get(property.get_key());
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->item_types.push_back(item_type);
			}
		});
	} else if (tag == "enchantments") {
		for (const std::string &value : values) {
			this->enchantments.push_back(enchantment::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const enchantment *enchantment = enchantment::get(property.get_key());
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->enchantments.push_back(enchantment);
			}
		});
	} else if (tag == "spells") {
		for (const std::string &value : values) {
			this->spells.push_back(spell::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const spell *spell = spell::get(property.get_key());
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->spells.push_back(spell);
			}
		});
	} else if (tag == "recipes") {
		for (const std::string &value : values) {
			this->recipes.push_back(recipe::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const recipe *recipe = recipe::get(property.get_key());
			const int weight = std::stoi(property.get_value());
			for (int i = 0; i < weight; ++i) {
				this->recipes.push_back(recipe);
			}
		});
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void item_creation_type::check() const
{
	if (this->item_creation_subtypes.empty() && this->item_types.empty()) {
		throw std::runtime_error(std::format("Item creation type \"{}\" has neither item creation subtypes nor information for creating items directly.", this->get_identifier()));
	}
}

qunique_ptr<item> item_creation_type::create_item(const site *creation_site) const
{
	assert_throw(creation_site != nullptr);

	std::vector<std::variant<const item_creation_type *, const item_type *>> bases;
	vector::merge(bases, this->item_creation_subtypes);
	vector::merge(bases, this->item_types);

	const auto &chosen_base = vector::get_random(bases);

	if (std::holds_alternative<const metternich::item_creation_type *>(chosen_base)) {
		return std::get<const metternich::item_creation_type *>(chosen_base)->create_item(creation_site);
	}

	assert_throw(std::holds_alternative<const item_type *>(chosen_base));

	const item_type *item_type = std::get<const metternich::item_type *>(chosen_base);

	std::vector<std::variant<const enchantment *, const spell *, const recipe *>> properties;

	if (!this->enchantments.empty()) {
		std::vector<const enchantment *> enchantments = this->enchantments;
		std::erase_if(enchantments, [this, creation_site, item_type](const enchantment *enchantment) {
			if (enchantment->get_creation_site_conditions() != nullptr && !enchantment->get_creation_site_conditions()->check(creation_site, read_only_context(creation_site))) {
				return true;
			}

			if (!enchantment->get_item_types().contains(item_type) && !enchantment->get_item_classes().contains(item_type->get_item_class())) {
				return true;
			}

			return false;
		});

		vector::merge(properties, std::move(enchantments));
	}

	if (!this->spells.empty()) {
		vector::merge(properties, this->spells);
	}

	if (!this->recipes.empty()) {
		vector::merge(properties, this->recipes);
	}

	const enchantment *enchantment = nullptr;
	const spell *spell = nullptr;
	const recipe *recipe = nullptr;

	const auto &chosen_property = vector::get_random(properties);

	if (std::holds_alternative<const metternich::enchantment *>(chosen_property)) {
		enchantment = std::get<const metternich::enchantment *>(chosen_property);
	} else if (std::holds_alternative<const metternich::spell *>(chosen_property)) {
		spell = std::get<const metternich::spell *>(chosen_property);
	} else if (std::holds_alternative<const metternich::recipe *>(chosen_property)) {
		recipe = std::get<const metternich::recipe *>(chosen_property);
	}

	return make_qunique<item>(item_type, nullptr, enchantment, spell, recipe);
}


bool item_creation_type::can_create_item_type(const item_type *item_type) const
{
	if (vector::contains(this->item_types, item_type)) {
		return true;
	}

	for (const item_creation_type *item_creation_subtype : this->item_creation_subtypes) {
		if (item_creation_subtype->can_create_item_type(item_type)) {
			return true;
		}
	}

	return false;
}

}
