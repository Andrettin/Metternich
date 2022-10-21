#include "metternich.h"

#include "country/landed_title.h"

#include "country/landed_title_tier.h"
#include "map/site.h"
#include "util/assert_util.h"
#include "util/gender.h"

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

void landed_title::process_character_title_names(character_title_name_map &character_title_names, const gsml_data &scope)
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
}

void landed_title::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "title_names") {
		landed_title::process_title_names(this->title_names, scope);
	} else if (tag == "character_title_names") {
		landed_title::process_character_title_names(this->character_title_names, scope);
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

void landed_title::set_site(const metternich::site *site)
{
	this->site = site;
	this->default_tier = landed_title_tier::barony; //resource sites are baronies by default
}

}
