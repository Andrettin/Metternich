#include "metternich.h"

#include "character/character_attribute.h"

#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {
	
void character_attribute::initialize_all()
{
	data_type::initialize_all();

	character_attribute::sort_instances([](const character_attribute *lhs, const character_attribute *rhs) {
		if (lhs->is_subattribute() != rhs->is_subattribute()) {
			//main attributes should come first in the list; this is important for e.g. character attribute generation order
			return !lhs->is_subattribute();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

character_attribute::character_attribute(const std::string &identifier) : character_stat(identifier)
{
}

character_attribute::~character_attribute()
{
}

void character_attribute::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "rating_ranges") {
		scope.for_each_property([this](const gsml_property &property) {
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
	} else if (tag == "exceptional_value_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int value = std::stoi(child_tag);

			child_scope.for_each_child([this, value](const gsml_data &grandchild_scope) {
				const std::string &grandchild_tag = grandchild_scope.get_tag();
				const int exceptional_value = std::stoi(grandchild_tag);

				assert_throw(exceptional_value >= 1);
				assert_throw(exceptional_value <= 99);

				if (!this->exceptional_value_modifiers[value].contains(exceptional_value)) {
					this->exceptional_value_modifiers[value][exceptional_value] = std::make_unique<metternich::modifier<const character>>();
				}
				this->exceptional_value_modifiers[value][exceptional_value]->process_gsml_data(grandchild_scope);
			});
		});
	} else {
		character_stat::process_gsml_scope(scope);
	}
}

void character_attribute::initialize()
{
	if (this->base_attribute != nullptr) {
		this->base_attribute->add_subattribute(this);
	}

	character_stat::initialize();
}

const modifier<const character> *character_attribute::get_exceptional_value_modifier(const int value, const int exceptional_value) const
{
	const auto find_iterator = this->exceptional_value_modifiers.find(value);
	if (find_iterator != this->exceptional_value_modifiers.end()) {
		const auto sub_find_iterator = find_iterator->second.upper_bound(exceptional_value);
		assert_throw(sub_find_iterator != find_iterator->second.begin());
		return std::prev(sub_find_iterator)->second.get();
	}

	return nullptr;
}

}
