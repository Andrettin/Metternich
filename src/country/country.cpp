#include "metternich.h"

#include "country/country.h"

#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/country_tier.h"
#include "country/country_tier_data.h"
#include "country/country_turn_data.h"
#include "country/country_type.h"
#include "country/culture.h"
#include "country/government_group.h"
#include "country/government_type.h"
#include "country/religion.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/site.h"
#include "technology/technology.h"
#include "time/era.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/string_util.h"

namespace metternich {

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
		government_type::process_title_name_scope(this->short_names, scope);
	} else if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "ruler_title_names") {
		government_type::process_ruler_title_name_scope(this->ruler_title_names, scope);
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

	if (this->is_tribe()) {
		this->short_name = true;
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

	this->get_game_data()->set_tier(this->get_default_tier());
	this->get_game_data()->set_government_type(this->get_default_government_type());
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
	if (government_type == nullptr) {
		return this->get_name();
	}

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

std::string country::get_titled_name(const government_type *government_type, const country_tier tier, const religion *religion) const
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

	if (this->has_short_name()) {
		return this->get_name();
	}

	const std::string title_name = this->get_title_name(government_type, tier, religion);
	const std::string country_name = this->get_name();
	if (this->definite_article) {
		return std::format("{} of the {}", title_name, country_name);
	} else {
		return std::format("{} of {}", title_name, country_name);
	}
}

const std::string &country::get_title_name(const government_type *government_type, const country_tier tier, const religion *religion) const
{
	if (government_type == nullptr) {
		return country_tier_data::get(tier)->get_name();
	}

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

	if (government_type->get_group()->is_religious()) {
		const std::string &religion_title_name = religion->get_title_name(government_type, tier);
		if (!religion_title_name.empty()) {
			return religion_title_name;
		}
	}

	const std::string &culture_title_name = this->get_culture()->get_title_name(government_type, tier);
	if (!culture_title_name.empty()) {
		return culture_title_name;
	}

	assert_throw(government_type != nullptr);

	return government_type->get_title_name(tier);
}

const std::string &country::get_ruler_title_name(const government_type *government_type, const country_tier tier, const gender gender, const religion *religion) const
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

	if (government_type->get_group()->is_religious()) {
		const std::string &religion_ruler_title_name = religion->get_ruler_title_name(government_type, tier, gender);
		if (!religion_ruler_title_name.empty()) {
			return religion_ruler_title_name;
		}
	}

	const std::string &culture_ruler_title_name = this->get_culture()->get_ruler_title_name(government_type, tier, gender);
	if (!culture_ruler_title_name.empty()) {
		return culture_ruler_title_name;
	}

	assert_throw(government_type != nullptr);

	return government_type->get_ruler_title_name(tier, gender);
}

bool country::can_declare_war() const
{
	return this->get_type() == country_type::great_power;
}

std::vector<const technology *> country::get_available_technologies() const
{
	std::vector<const technology *> technologies;

	for (const technology *technology : technology::get_all()) {
		if (!technology->is_available_for_country(this)) {
			continue;
		}

		technologies.push_back(technology);
	}

	std::sort(technologies.begin(), technologies.end(), technology_compare());

	return technologies;
}

QVariantList country::get_available_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_technologies());
}

}
