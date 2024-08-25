#include "metternich.h"

#include "country/government_type.h"

#include "country/country_tier.h"
#include "country/government_group.h"
#include "country/policy.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"
#include "util/gender.h"

namespace metternich {

void government_type::process_title_name_scope(std::map<government_variant, title_name_map> &title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		government_variant government_variant{};
		const government_group *government_group = government_group::try_get(key);
		if (government_group != nullptr) {
			government_variant = government_group;
		} else {
			government_variant = government_type::get(key);
		}

		title_names[government_variant][country_tier::none] = value;
	});

	scope.for_each_child([&](const gsml_data &child_scope) {
		government_variant government_variant{};
		const government_group *government_group = government_group::try_get(child_scope.get_tag());
		if (government_group != nullptr) {
			government_variant = government_group;
		} else {
			government_variant = government_type::get(child_scope.get_tag());
		}

		government_type::process_title_name_scope(title_names[government_variant], child_scope);
	});
}

void government_type::process_title_name_scope(title_name_map &title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		title_names[tier] = value;
	});
}

void government_type::process_ruler_title_name_scope(std::map<government_variant, ruler_title_name_map> &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_child([&](const gsml_data &child_scope) {
		government_variant government_variant{};
		const government_group *government_group = government_group::try_get(child_scope.get_tag());
		if (government_group != nullptr) {
			government_variant = government_group;
		} else {
			government_variant = government_type::get(child_scope.get_tag());
		}

		government_type::process_ruler_title_name_scope(ruler_title_names[government_variant], child_scope);
	});
}

void government_type::process_ruler_title_name_scope(ruler_title_name_map &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		if (enum_converter<country_tier>::has_value(key)) {
			const country_tier tier = enum_converter<country_tier>::to_enum(key);
			ruler_title_names[tier][gender::none] = value;
		} else {
			const gender gender = enum_converter<metternich::gender>::to_enum(key);
			ruler_title_names[country_tier::none][gender] = value;
		}
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

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "ruler_title_names") {
		government_type::process_ruler_title_name_scope(this->ruler_title_names, scope);
	} else if (tag == "min_policy_values") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const policy *policy = policy::get(key);
			const int value_int = std::stoi(value);
			assert_throw(value_int >= policy::min_value);
			this->min_policy_values[policy] = value_int;
		});
	} else if (tag == "max_policy_values") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const policy *policy = policy::get(key);
			const int value_int = std::stoi(value);
			assert_throw(value_int <= policy::max_value);
			this->max_policy_values[policy] = value_int;
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void government_type::check() const
{
	if (this->get_group() == nullptr) {
		throw std::runtime_error(std::format("Government type \"{}\" has no government group.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Government type \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
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

int government_type::get_min_policy_value(const policy *policy) const
{
	const auto find_iterator = this->min_policy_values.find(policy);
	if (find_iterator != this->min_policy_values.end()) {
		return find_iterator->second;
	}

	return policy::min_value;
}

int government_type::get_max_policy_value(const policy *policy) const
{
	const auto find_iterator = this->max_policy_values.find(policy);
	if (find_iterator != this->max_policy_values.end()) {
		return find_iterator->second;
	}

	return policy::max_value;
}

}
