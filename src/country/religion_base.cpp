#include "metternich.h"

#include "country/religion_base.h"

#include "country/country_tier.h"
#include "country/government_type.h"
#include "util/gender.h"
#include "util/string_util.h"

namespace metternich {

void religion_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "ruler_title_names") {
		government_type::process_ruler_title_name_scope(this->ruler_title_names, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

const std::string &religion_base::get_title_name(const government_type *government_type, const country_tier tier) const
{
	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator == this->title_names.end()) {
		find_iterator = this->title_names.find(government_type->get_group());
	}

	if (find_iterator != this->title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(country_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	return string::empty_str;
}

const std::string &religion_base::get_ruler_title_name(const government_type *government_type, const country_tier tier, const gender gender) const
{
	auto find_iterator = this->ruler_title_names.find(government_type);
	if (find_iterator == this->ruler_title_names.end()) {
		find_iterator = this->ruler_title_names.find(government_type->get_group());
	}

	if (find_iterator != this->ruler_title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(country_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			auto sub_sub_find_iterator = sub_find_iterator->second.find(gender);
			if (sub_sub_find_iterator == sub_find_iterator->second.end()) {
				sub_sub_find_iterator = sub_find_iterator->second.find(gender::none);
			}

			if (sub_sub_find_iterator != sub_find_iterator->second.end()) {
				return sub_sub_find_iterator->second;
			}
		}
	}

	return string::empty_str;
}

}
