#include "metternich.h"

#include "country/government_type.h"

#include "country/country_tier.h"
#include "country/government_group.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/gender.h"

namespace metternich {

void government_type::process_title_names(title_name_map &title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		title_names[tier] = value;
	});
}

void government_type::process_ruler_title_names(ruler_title_name_map &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		ruler_title_names[tier][gender::none] = value;
	});

	scope.for_each_child([&](const gsml_data &child_scope) {
		const country_tier tier = enum_converter<country_tier>::to_enum(child_scope.get_tag());

		government_type::process_ruler_title_name_scope(ruler_title_names[tier], child_scope);
	});
}

void government_type::process_ruler_title_name_scope(std::map<gender, std::string> &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const gender gender = enum_converter<archimedes::gender>::to_enum(key);
		ruler_title_names[gender] = value;
	});
}

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
	} else if (tag == "title_names") {
		government_type::process_title_names(this->title_names, scope);
	} else if (tag == "ruler_title_names") {
		government_type::process_ruler_title_names(this->ruler_title_names, scope);
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
	if (this->get_group() == nullptr) {
		throw std::runtime_error(std::format("Government type \"{}\" has no government group.", this->get_identifier()));
	}

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

	return this->get_group()->get_title_name(tier);
}

const std::string &government_type::get_ruler_title_name(const country_tier tier, const gender gender) const
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

	return this->get_group()->get_ruler_title_name(tier, gender);
}

QString government_type::get_modifier_string(metternich::country *country) const
{
	return QString::fromStdString(this->get_modifier()->get_string(country));
}

}
