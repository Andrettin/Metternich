#include "metternich.h"

#include "domain/government_group.h"

#include "domain/domain_tier.h"
#include "domain/domain_tier_data.h"
#include "domain/government_type.h"
#include "domain/office.h"
#include "database/defines.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/string_util.h"

namespace metternich {

void government_group::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "site_title_names") {
		government_type::process_site_title_name_scope(this->site_title_names, scope);
	} else if (tag == "office_title_names") {
		government_type::process_office_title_name_scope(this->office_title_names, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

const std::string &government_group::get_title_name(const domain_tier tier) const
{
	const std::string &title_name = government_type::get_title_name(this->title_names, tier);
	if (!title_name.empty()) {
		return title_name;
	}

	return domain_tier_data::get(tier)->get_name();
}

const std::string &government_group::get_site_title_name(const int tier) const
{
	const auto find_iterator = this->site_title_names.find(tier);
	if (find_iterator != this->site_title_names.end()) {
		return find_iterator->second;
	}

	return string::empty_str;
}

const std::string &government_group::get_office_title_name(const metternich::office *office, const domain_tier tier, const gender gender) const
{
	const std::string &office_title_name = government_type::get_office_title_name(this->office_title_names, office, tier, gender);
	if (!office_title_name.empty()) {
		return office_title_name;
	}

	if (office->is_ruler()) {
		switch (tier) {
			case domain_tier::barony:
				if (gender == gender::female) {
					static const std::string str = "Baroness";
					return str;
				} else {
					static const std::string str = "Baron";
					return str;
				}
			case domain_tier::county:
				if (gender == gender::female) {
					static const std::string str = "Countess";
					return str;
				} else {
					static const std::string str = "Count";
					return str;
				}
			case domain_tier::duchy:
				if (gender == gender::female) {
					static const std::string str = "Duchess";
					return str;
				} else {
					static const std::string str = "Duke";
					return str;
				}
			case domain_tier::kingdom:
				if (gender == gender::female) {
					static const std::string str = "Queen";
					return str;
				} else {
					static const std::string str = "King";
					return str;
				}
			case domain_tier::empire:
				if (gender == gender::female) {
					static const std::string str = "Empress";
					return str;
				} else {
					static const std::string str = "Emperor";
					return str;
				}
			default:
				break;
		}
	}

	return office->get_name();
}

}
