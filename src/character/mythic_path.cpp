#include "metternich.h"

#include "character/mythic_path.h"

#include "util/string_util.h"

namespace metternich {

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

}
