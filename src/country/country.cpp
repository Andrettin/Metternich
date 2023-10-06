#include "metternich.h"

#include "country/country.h"

#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/country_tier.h"
#include "country/country_turn_data.h"
#include "country/country_type.h"
#include "country/government_group.h"
#include "country/government_type.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/site.h"
#include "time/era.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/string_util.h"

namespace metternich {

void country::process_title_names(title_name_map &title_names, const gsml_data &scope)
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

		country::process_title_name_scope(title_names[government_variant], child_scope);
	});
}

void country::process_title_name_scope(std::map<country_tier, std::string> &title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		title_names[tier] = value;
	});
}

void country::process_ruler_title_names(ruler_title_name_map &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_child([&](const gsml_data &child_scope) {
		government_variant government_variant{};
		const government_group *government_group = government_group::try_get(child_scope.get_tag());
		if (government_group != nullptr) {
			government_variant = government_group;
		} else {
			government_variant = government_type::get(child_scope.get_tag());
		}

		country::process_ruler_title_name_scope(ruler_title_names[government_variant], child_scope);
	});
}

void country::process_ruler_title_name_scope(std::map<country_tier, std::map<gender, std::string>> &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		ruler_title_names[tier][gender::none] = value;
	});

	scope.for_each_child([&](const gsml_data &child_scope) {
		const country_tier tier = enum_converter<country_tier>::to_enum(child_scope.get_tag());

		country::process_ruler_title_name_scope(ruler_title_names[tier], child_scope);
	});
}

void country::process_ruler_title_name_scope(std::map<gender, std::string> &ruler_title_names, const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const gender gender = enum_converter<archimedes::gender>::to_enum(key);
		ruler_title_names[gender] = value;
	});
}

country::country(const std::string &identifier)
	: named_data_entry(identifier), type(country_type::minor_nation), default_tier(country_tier::none), min_tier(country_tier::none), max_tier(country_tier::none)
{
	this->reset_game_data();
}

country::~country()
{
}

void country::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "eras") {
		for (const std::string &value : values) {
			this->eras.push_back(era::get(value));
		}
	} else if (tag == "short_names") {
		country::process_title_names(this->short_names, scope);
	} else if (tag == "title_names") {
		country::process_title_names(this->title_names, scope);
	} else if (tag == "ruler_title_names") {
		country::process_ruler_title_names(this->ruler_title_names, scope);
	} else if (tag == "core_provinces") {
		for (const std::string &value : values) {
			this->core_provinces.push_back(province::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void country::initialize()
{
	if (this->get_min_tier() == country_tier::none) {
		this->min_tier = this->get_default_tier();
	}

	if (this->get_max_tier() == country_tier::none) {
		this->max_tier = this->get_default_tier();
	}

	for (province *province : this->get_core_provinces()) {
		province->add_core_country(this);
	}

	named_data_entry::initialize();
}

void country::check() const
{
	if (this->get_default_tier() == country_tier::none) {
		throw std::runtime_error(std::format("Country \"{}\" has no default tier.", this->get_identifier()));
	}

	if (this->get_min_tier() == country_tier::none) {
		throw std::runtime_error(std::format("Country \"{}\" has no min tier.", this->get_identifier()));
	}

	if (this->get_max_tier() == country_tier::none) {
		throw std::runtime_error(std::format("Country \"{}\" has no max tier.", this->get_identifier()));
	}

	if (this->get_culture() == nullptr) {
		throw std::runtime_error(std::format("Country \"{}\" has no culture.", this->get_identifier()));
	}

	if (this->get_default_religion() == nullptr) {
		throw std::runtime_error(std::format("Country \"{}\" has no default religion.", this->get_identifier()));
	}

	if (this->get_default_government_type() == nullptr) {
		throw std::runtime_error(std::format("Country \"{}\" has no default government type.", this->get_identifier()));
	}

	if (this->get_default_capital() == nullptr) {
		throw std::runtime_error(std::format("Country \"{}\" has no default capital.", this->get_identifier()));
	}

	if (!this->get_default_capital()->is_settlement()) {
		throw std::runtime_error(std::format("The default capital for country \"{}\" (\"{}\") is not a settlement.", this->get_identifier(), this->get_default_capital()->get_identifier()));
	}

	assert_throw(this->get_color().isValid());
}

data_entry_history *country::get_history_base()
{
	return this->history.get();
}

void country::reset_history()
{
	this->history = make_qunique<country_history>(this);
}

void country::reset_game_data()
{
	this->game_data = make_qunique<country_game_data>(this);
	this->get_game_data()->initialize_building_slots();

	this->reset_turn_data();
}

void country::reset_turn_data()
{
	this->turn_data = make_qunique<country_turn_data>(this);
	emit turn_data_changed();
}

bool country::is_great_power() const
{
	return this->get_type() == country_type::great_power;
}

bool country::is_tribe() const
{
	return this->get_type() == country_type::tribe;
}

const QColor &country::get_color() const
{
	if (this->get_type() != country_type::great_power) {
		return defines::get()->get_minor_nation_color();
	}

	return this->color;
}

const std::string &country::get_name(const government_type *government_type, const country_tier tier) const
{
	auto find_iterator = this->short_names.find(government_type);
	if (find_iterator == this->short_names.end()) {
		find_iterator = this->short_names.find(government_type->get_group());
	}

	if (find_iterator != this->short_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(country_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	return this->get_name();
}

const std::string &country::get_titled_name(const government_type *government_type, const country_tier tier) const
{
	auto find_iterator = this->short_names.find(government_type);
	if (find_iterator == this->short_names.end()) {
		find_iterator = this->short_names.find(government_type->get_group());
	}

	if (find_iterator != this->short_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(country_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	if (this->is_tribe()) {
		return this->get_name();
	}

	return std::format("{} of {}", this->get_title_name(government_type, tier), this->get_name());
}

const std::string &country::get_title_name(const government_type *government_type, const country_tier tier) const
{
	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator == this->title_names.end()) {
		find_iterator = this->title_names.find(government_type->get_group());
	}

	if (find_iterator != this->title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	assert_throw(government_type != nullptr);

	return government_type->get_title_name(tier);
}

const std::string &country::get_ruler_title_name(const government_type *government_type, const country_tier tier, const gender gender) const
{
	auto find_iterator = this->ruler_title_names.find(government_type);
	if (find_iterator == this->ruler_title_names.end()) {
		find_iterator = this->ruler_title_names.find(government_type->get_group());
	}

	if (find_iterator != this->ruler_title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(tier);
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

	assert_throw(government_type != nullptr);

	return government_type->get_ruler_title_name(tier, gender);
}

bool country::can_declare_war() const
{
	return this->get_type() == country_type::great_power;
}

}
