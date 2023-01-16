#include "metternich.h"

#include "country/landed_title.h"

#include "country/country.h"
#include "country/landed_title_game_data.h"
#include "country/landed_title_tier.h"
#include "map/site.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/string_util.h"

namespace metternich {

//must be initialized after sites, in their initialization function landed titles may be created
const std::set<std::string> landed_title::database_dependencies = { site::class_identifier };

void landed_title::process_title_names(title_name_map &title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const landed_title_tier tier = enum_converter<landed_title_tier>::to_enum(key);
		title_names[tier] = value;
	});
}

void landed_title::process_character_title_names(ruler_title_name_map &character_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const landed_title_tier tier = enum_converter<landed_title_tier>::to_enum(key);
		character_title_names[tier][gender::none] = value;
	});

	scope.for_each_child([&](const gsml_data &child_scope) {
		const landed_title_tier tier = enum_converter<landed_title_tier>::to_enum(child_scope.get_tag());

		landed_title::process_character_title_name_scope(character_title_names[tier], child_scope);
	});
}

void landed_title::process_character_title_name_scope(std::map<gender, std::string> &character_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const gender gender = enum_converter<archimedes::gender>::to_enum(key);
		character_title_names[gender] = value;
	});
}

landed_title::landed_title(const std::string &identifier)
	: named_data_entry(identifier), default_tier(landed_title_tier::none), min_tier(landed_title_tier::none), max_tier(landed_title_tier::none)
{
	this->reset_game_data();
}

void landed_title::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "title_names") {
		landed_title::process_title_names(this->title_names, scope);
	} else if (tag == "character_title_names") {
		landed_title::process_character_title_names(this->ruler_title_names, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void landed_title::initialize()
{
	if (this->get_min_tier() == landed_title_tier::none) {
		this->min_tier = this->get_default_tier();
	}

	if (this->get_max_tier() == landed_title_tier::none) {
		this->max_tier = this->get_default_tier();
	}

	data_entry::initialize();
}

void landed_title::check() const
{
	assert_throw(this->get_country() != nullptr || this->get_site() != nullptr);
	assert_throw(this->get_default_tier() != landed_title_tier::none);
	assert_throw(this->get_min_tier() != landed_title_tier::none);
	assert_throw(this->get_max_tier() != landed_title_tier::none);
}

void landed_title::reset_game_data()
{
	this->game_data = make_qunique<landed_title_game_data>(this);
	emit game_data_changed();
}

void landed_title::set_country(const metternich::country *country)
{
	this->country = country;
	this->set_name(this->get_country()->get_name());
}

void landed_title::set_site(const metternich::site *site)
{
	this->site = site;
	this->default_tier = landed_title_tier::barony; //resource sites are baronies by default
	this->set_name(this->get_site()->get_name());
}

const std::string &landed_title::get_title_name(const landed_title_tier tier) const
{
	const auto find_iterator = this->title_names.find(tier);
	if (find_iterator != this->title_names.end()) {
		return find_iterator->second;
	}

	switch (tier) {
		case landed_title_tier::barony: {
			static const std::string str = "Barony";
			return str;
		}
		case landed_title_tier::viscounty: {
			static const std::string str = "Viscounty";
			return str;
		}
		case landed_title_tier::county: {
			static const std::string str = "County";
			return str;
		}
		case landed_title_tier::marquisate: {
			static const std::string str = "Marquisate";
			return str;
		}
		case landed_title_tier::duchy: {
			static const std::string str = "Duchy";
			return str;
		}
		case landed_title_tier::grand_duchy: {
			static const std::string str = "Grand Duchy";
			return str;
		}
		case landed_title_tier::kingdom: {
			static const std::string str = "Kingdom";
			return str;
		}
		case landed_title_tier::empire: {
			static const std::string str = "Empire";
			return str;
		}
		default:
			break;
	}

	return string::empty_str;
}

const std::string &landed_title::get_ruler_title_name(const landed_title_tier tier, const gender gender) const
{
	const auto find_iterator = this->ruler_title_names.find(tier);
	if (find_iterator != this->ruler_title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	switch (tier) {
		case landed_title_tier::barony:
			if (gender == gender::female) {
				static const std::string str = "Baroness";
				return str;
			} else {
				static const std::string str = "Baron";
				return str;
			}
		case landed_title_tier::viscounty:
			if (gender == gender::female) {
				static const std::string str = "Viscountess";
				return str;
			} else {
				static const std::string str = "Viscount";
				return str;
			}
		case landed_title_tier::county:
			if (gender == gender::female) {
				static const std::string str = "Countess";
				return str;
			} else {
				static const std::string str = "Count";
				return str;
			}
		case landed_title_tier::marquisate:
			if (gender == gender::female) {
				static const std::string str = "Marquise";
				return str;
			} else {
				static const std::string str = "Marquis";
				return str;
			}
		case landed_title_tier::duchy:
			if (gender == gender::female) {
				static const std::string str = "Duchess";
				return str;
			} else {
				static const std::string str = "Duke";
				return str;
			}
		case landed_title_tier::grand_duchy:
			if (gender == gender::female) {
				static const std::string str = "Grand Duchess";
				return str;
			} else {
				static const std::string str = "Grand Duke";
				return str;
			}
		case landed_title_tier::kingdom:
			if (gender == gender::female) {
				static const std::string str = "Queen";
				return str;
			} else {
				static const std::string str = "King";
				return str;
			}
		case landed_title_tier::empire:
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
