#include "metternich.h"

#include "map/province_attribute.h"

#include "script/modifier.h"

namespace metternich {

province_attribute::province_attribute(const std::string &identifier) : named_data_entry(identifier)
{
}

province_attribute::~province_attribute()
{
}

void province_attribute::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "value_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int value = std::stoi(child_tag);
			if (!this->value_modifiers.contains(value)) {
				this->value_modifiers[value] = std::make_unique<metternich::modifier<const province>>();
			}
			this->value_modifiers[value]->process_gsml_data(child_scope);
		});
	} else if (tag == "recurring_value_modifiers") {
		static constexpr int max_value = std::numeric_limits<uint8_t>::max();

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int value_interval = std::stoi(child_tag);
			for (int i = value_interval; i <= max_value; i += value_interval) {
				if (!this->value_modifiers.contains(i)) {
					this->value_modifiers[i] = std::make_unique<metternich::modifier<const province>>();
				}
				this->value_modifiers[i]->process_gsml_data(child_scope);
			}
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

}
