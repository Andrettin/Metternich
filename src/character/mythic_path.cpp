#include "metternich.h"

#include "character/mythic_path.h"

#include "script/modifier.h"
#include "util/string_util.h"

namespace metternich {

mythic_path::mythic_path(const std::string &identifier) : named_data_entry(identifier)
{
}

mythic_path::~mythic_path()
{
}

void mythic_path::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "rank_tiers") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->rank_tiers[key] = std::stoi(value);
		});
	} else if (tag == "tier_title_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->tier_title_names[std::stoi(key)] = value;
		});
	} else if (tag == "tier_modifiers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int tier = std::stoi(child_tag);
			if (!this->tier_modifiers.contains(tier)) {
				this->tier_modifiers[tier] = std::make_unique<metternich::modifier<const character>>();
			}
			this->tier_modifiers[tier]->process_gsml_data(child_scope);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

const std::string &mythic_path::get_tier_title_name(const int mythic_tier) const
{
	if (!this->tier_title_names.empty()) {
		auto find_iterator = this->tier_title_names.upper_bound(mythic_tier);
		if (find_iterator != this->tier_title_names.begin()) {
			--find_iterator; //get the one just before
			return find_iterator->second;
		}
	}

	return string::empty_str;
}

std::string mythic_path::get_tier_modifier_string(const int tier, const metternich::character *character) const
{
	std::string str;

	const modifier<const metternich::character> *tier_modifier = this->get_tier_modifier(tier);
	if (tier_modifier != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += tier_modifier->get_string(character);
	}

	return str;
}

}
