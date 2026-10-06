#include "metternich.h"

#include "character/character_package.h"

#include "database/defines.h"
#include "economy/commodity.h"
#include "item/item_type.h"
#include "script/condition/and_condition.h"
#include "spell/spell.h"

namespace metternich {

character_package::character_package(const std::string &identifier) : data_entry(identifier)
{
}

character_package::~character_package()
{
}

void character_package::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "wealth") {
		const auto &[wealth_variant, wealth_unit] = defines::get()->get_wealth_commodity()->string_to_value_variant_with_unit(value);
		this->wealth_variant = wealth_variant;
		this->wealth_unit = wealth_unit;
	} else {
		data_entry::process_gsml_property(property);
	}
}

void character_package::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "items") {
		for (const std::string &value : values) {
			this->items.push_back(item_type::get(value));
		}
	} else if (tag == "spells") {
		for (const std::string &value : values) {
			this->spells.push_back(spell::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

}
