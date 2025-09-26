#include "metternich.h"

#include "character/character_attribute.h"

#include "script/modifier.h"

namespace metternich {

character_attribute::character_attribute(const std::string &identifier) : named_data_entry(identifier)
{
}

character_attribute::~character_attribute()
{
}

void character_attribute::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "value_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level = std::stoi(child_tag);
			auto modifier = std::make_unique<metternich::modifier<const character>>();
			modifier->process_gsml_data(child_scope);
			this->value_modifiers[level] = std::move(modifier);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}


}
