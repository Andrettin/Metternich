#include "metternich.h"

#include "country/country.h"

#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/country_tier.h"
#include "country/country_turn_data.h"
#include "country/country_type.h"
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
		const country_tier tier = enum_converter<country_tier>::to_enum(key);
		title_names[tier] = value;
	});
}

void country::process_ruler_title_names(ruler_title_name_map &ruler_title_names, const gsml_data &scope)
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

const std::string &country::get_title_name(const country_tier tier) const
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

const std::string &country::get_ruler_title_name(const country_tier tier, const gender gender) const
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

const population_class *country::get_default_population_class() const
{
	if (this->is_tribe()) {
		return defines::get()->get_default_tribal_population_class();
	} else {
		return defines::get()->get_default_population_class();
	}
}

bool country::can_declare_war() const
{
	return this->get_type() == country_type::great_power;
}

}
