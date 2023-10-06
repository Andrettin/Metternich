#include "metternich.h"

#include "country/government_type.h"

#include "country/country_tier.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/gender.h"
#include "util/string_util.h"

namespace metternich {

government_type::government_type(const std::string &identifier) : named_data_entry(identifier)
{
}

government_type::~government_type()
{
}

void government_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void government_type::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_government_type(this);
	}

	named_data_entry::initialize();
}

void government_type::check() const
{
	if (this->get_modifier() == nullptr) {
		throw std::runtime_error(std::format("Government type \"{}\" does not have a modifier.", this->get_identifier()));
	}
}

const std::string &government_type::get_title_name(const country_tier tier) const
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

const std::string &government_type::get_ruler_title_name(const country_tier tier, const gender gender) const
{
	const auto find_iterator = this->ruler_title_names.find(tier);
	if (find_iterator != this->ruler_title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(gender);
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

QString government_type::get_modifier_string(metternich::country *country) const
{
	return QString::fromStdString(this->get_modifier()->get_string(country));
}

}
