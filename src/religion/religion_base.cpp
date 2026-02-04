#include "metternich.h"

#include "religion/religion_base.h"

#include "domain/domain_tier.h"
#include "domain/government_type.h"
#include "util/gender.h"
#include "util/string_util.h"

namespace metternich {

void religion_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "office_title_names") {
		government_type::process_office_title_name_scope(this->office_title_names, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

const std::string &religion_base::get_title_name(const government_type *government_type, const domain_tier tier) const
{
	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator == this->title_names.end()) {
		find_iterator = this->title_names.find(government_type->get_group());
	}

	if (find_iterator != this->title_names.end()) {
		return government_type::get_title_name(find_iterator->second, tier);
	}

	return string::empty_str;
}

const std::string &religion_base::get_office_title_name(const office *office, const government_type *government_type, const domain_tier tier, const gender gender) const
{
	const auto office_find_iterator = this->office_title_names.find(office);
	if (office_find_iterator != this->office_title_names.end()) {
		auto government_find_iterator = office_find_iterator->second.find(government_type);
		if (government_find_iterator == office_find_iterator->second.end()) {
			government_find_iterator = office_find_iterator->second.find(government_type->get_group());
		}

		if (government_find_iterator != office_find_iterator->second.end()) {
			const std::string &office_title_name = government_type::get_office_title_name(government_find_iterator->second, tier, gender);
			if (!office_title_name.empty()) {
				return office_title_name;
			}
		}
	}

	return string::empty_str;
}

}
