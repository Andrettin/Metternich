#include "metternich.h"

#include "country/government_group.h"

#include "country/country_tier.h"
#include "util/gender.h"
#include "util/string_util.h"

namespace metternich {

void government_group::process_title_names(title_name_map &title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		title_names[tier] = value;
	});
}

void government_group::process_ruler_title_names(ruler_title_name_map &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		ruler_title_names[tier][gender::none] = value;
	});

	scope.for_each_child([&](const gsml_data &child_scope) {
		const country_tier tier = enum_converter<country_tier>::to_enum(child_scope.get_tag());

		government_group::process_ruler_title_name_scope(ruler_title_names[tier], child_scope);
	});
}

void government_group::process_ruler_title_name_scope(std::map<gender, std::string> &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const gender gender = enum_converter<archimedes::gender>::to_enum(key);
		ruler_title_names[gender] = value;
	});
}

void government_group::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "title_names") {
		government_group::process_title_names(this->title_names, scope);
	} else if (tag == "ruler_title_names") {
		government_group::process_ruler_title_names(this->ruler_title_names, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

const std::string &government_group::get_title_name(const country_tier tier) const
{
	const auto find_iterator = this->title_names.find(tier);
	if (find_iterator != this->title_names.end()) {
		return find_iterator->second;
	}

	switch (tier) {
		case country_tier::barony: {
			static const std::string str = "Barony";
			return str;
		}
		case country_tier::county: {
			static const std::string str = "County";
			return str;
		}
		case country_tier::duchy: {
			static const std::string str = "Duchy";
			return str;
		}
		case country_tier::kingdom: {
			static const std::string str = "Kingdom";
			return str;
		}
		case country_tier::empire: {
			static const std::string str = "Empire";
			return str;
		}
		default:
			break;
	}

	return string::empty_str;
}

const std::string &government_group::get_ruler_title_name(const country_tier tier, const gender gender) const
{
	const auto find_iterator = this->ruler_title_names.find(tier);
	if (find_iterator != this->ruler_title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}

		sub_find_iterator = find_iterator->second.find(gender::none);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	switch (tier) {
		case country_tier::barony:
			if (gender == gender::female) {
				static const std::string str = "Baroness";
				return str;
			} else {
				static const std::string str = "Baron";
				return str;
			}
		case country_tier::county:
			if (gender == gender::female) {
				static const std::string str = "Countess";
				return str;
			} else {
				static const std::string str = "Count";
				return str;
			}
		case country_tier::duchy:
			if (gender == gender::female) {
				static const std::string str = "Duchess";
				return str;
			} else {
				static const std::string str = "Duke";
				return str;
			}
		case country_tier::kingdom:
			if (gender == gender::female) {
				static const std::string str = "Queen";
				return str;
			} else {
				static const std::string str = "King";
				return str;
			}
		case country_tier::empire:
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

	return string::empty_str;
}

}
