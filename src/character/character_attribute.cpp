#include "metternich.h"

#include "character/character_attribute.h"

#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/string_util.h"

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

	if (tag == "rating_ranges") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const std::vector<std::string> rating_values = string::split(value, '-');
			assert_throw(rating_values.size() >= 1 && rating_values.size() <= 2);

			std::pair<int, int> range {};
			range.first = std::stoi(rating_values.at(0));
			if (rating_values.size() == 2) {
				range.second = std::stoi(rating_values.at(1));
			} else {
				range.second = range.first;
			}

			this->rating_ranges[key] = std::move(range);
		});
	} else if (tag == "value_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int value = std::stoi(child_tag);
			if (!this->value_modifiers.contains(value)) {
				this->value_modifiers[value] = std::make_unique<metternich::modifier<const character>>();
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
					this->value_modifiers[i] = std::make_unique<metternich::modifier<const character>>();
				}
				this->value_modifiers[i]->process_gsml_data(child_scope);
			}
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

}
