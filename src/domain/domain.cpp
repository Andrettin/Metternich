#include "metternich.h"

#include "domain/domain.h"

#include "database/database.h"
#include "database/defines.h"
#include "domain/country_ai.h"
#include "domain/country_government.h"
#include "domain/country_turn_data.h"
#include "domain/country_type.h"
#include "domain/culture.h"
#include "domain/domain_game_data.h"
#include "domain/domain_history.h"
#include "domain/domain_tier.h"
#include "domain/domain_tier_data.h"
#include "domain/government_group.h"
#include "domain/government_type.h"
#include "domain/office.h"
#include "map/province.h"
#include "map/site.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "technology/technology.h"
#include "technology/technology_container.h"
#include "time/era.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/string_util.h"

namespace metternich {

domain::domain(const std::string &identifier)
	: named_data_entry(identifier), type(country_type::polity), default_tier(domain_tier::none), min_tier(domain_tier::none), max_tier(domain_tier::none)
{
	this->reset_game_data();
}

domain::~domain()
{
}

void domain::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "culture") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		this->cultures = { culture::get(value) };
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void domain::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "cultures") {
		for (const std::string &value : values) {
			this->cultures.push_back(culture::get(value));
		}
	} else if (tag == "conditional_flags") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			auto conditions = std::make_unique<and_condition<domain>>();
			conditions->process_gsml_data(child_scope);
			this->conditional_flags[child_tag] = std::move(conditions);
		});
	} else if (tag == "eras") {
		for (const std::string &value : values) {
			this->eras.push_back(era::get(value));
		}
	} else if (tag == "short_names") {
		government_type::process_title_name_scope(this->short_names, scope);
	} else if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "office_title_names") {
		government_type::process_office_title_name_scope(this->office_title_names, scope);
	} else if (tag == "core_provinces") {
		for (const std::string &value : values) {
			this->core_provinces.push_back(province::get(value));
		}
	} else if (tag == "core_holdings") {
		for (const std::string &value : values) {
			this->core_holdings.push_back(site::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void domain::initialize()
{
	if (this->get_min_tier() == domain_tier::none) {
		this->min_tier = this->get_default_tier();
	}

	if (this->get_max_tier() == domain_tier::none) {
		this->max_tier = this->get_default_tier();
	}

	if (this->is_tribe() || this->is_clade()) {
		this->short_name = true;
	}

	for (province *province : this->get_core_provinces()) {
		province->add_core_country(this);
	}

	named_data_entry::initialize();
}

void domain::check() const
{
	if (!this->get_flag().empty()) {
		const std::filesystem::path flag_filepath = database::get()->get_graphics_path(this->get_module()) / "flags" / (this->get_flag() + ".svg");
		if (!std::filesystem::exists(flag_filepath)) {
			throw std::runtime_error(std::format("Flag \"{}\" does not exist.", this->get_flag()));
		}
	} else {
		//log::log_error(std::format("Domain \"{}\" has no flag.", this->get_identifier()));
	}

	for (const auto &[conditional_flag, conditions] : this->get_conditional_flags()) {
		const std::filesystem::path flag_filepath = database::get()->get_graphics_path(this->get_module()) / "flags" / (conditional_flag + ".svg");
		if (!std::filesystem::exists(flag_filepath)) {
			throw std::runtime_error(std::format("Flag \"{}\" does not exist.", conditional_flag));
		}
	}

	if (this->get_default_tier() == domain_tier::none) {
		throw std::runtime_error(std::format("Domain \"{}\" has no default tier.", this->get_identifier()));
	}

	if (this->get_min_tier() == domain_tier::none) {
		throw std::runtime_error(std::format("Domain \"{}\" has no min tier.", this->get_identifier()));
	}

	if (this->get_max_tier() == domain_tier::none) {
		throw std::runtime_error(std::format("Domain \"{}\" has no max tier.", this->get_identifier()));
	}

	if (this->get_cultures().empty()) {
		throw std::runtime_error(std::format("Domain \"{}\" has no cultures.", this->get_identifier()));
	}

	if (this->get_default_religion() == nullptr) {
		throw std::runtime_error(std::format("Domain \"{}\" has no default religion.", this->get_identifier()));
	}

	if (this->get_default_government_type() == nullptr) {
		throw std::runtime_error(std::format("Domain \"{}\" has no default government type.", this->get_identifier()));
	}

	if (this->get_default_capital() == nullptr) {
		throw std::runtime_error(std::format("Domain \"{}\" has no default capital.", this->get_identifier()));
	}

	if (!this->get_default_capital()->is_settlement()) {
		throw std::runtime_error(std::format("The default capital for domain \"{}\" (\"{}\") is not a settlement.", this->get_identifier(), this->get_default_capital()->get_identifier()));
	}

	assert_throw(this->get_color().isValid());
}

data_entry_history *domain::get_history_base()
{
	return this->history.get();
}

void domain::reset_history()
{
	this->history = make_qunique<domain_history>(this);
}

void domain::reset_game_data()
{
	this->game_data = make_qunique<domain_game_data>(this);

	this->reset_turn_data();
	this->reset_ai();

	this->get_game_data()->set_tier(this->get_default_tier());
	this->get_game_data()->set_government_type(this->get_default_government_type());
}

country_economy *domain::get_economy() const
{
	return this->get_game_data()->get_economy();
}

country_government *domain::get_government() const
{
	return this->get_game_data()->get_government();
}

country_military *domain::get_military() const
{
	return this->get_game_data()->get_military();
}

country_technology *domain::get_technology() const
{
	return this->get_game_data()->get_technology();
}

void domain::reset_turn_data()
{
	this->turn_data = make_qunique<country_turn_data>(this);
	emit turn_data_changed();
}

void domain::reset_ai()
{
	this->ai = make_qunique<country_ai>(this);
	emit ai_changed();
}

bool domain::is_playable() const
{
	return this->get_type() == country_type::polity;
}

bool domain::is_tribe() const
{
	return this->get_type() == country_type::tribe;
}

bool domain::is_clade() const
{
	return this->get_type() == country_type::clade;
}

const QColor &domain::get_color() const
{
	if (!this->color.isValid()) {
		return defines::get()->get_minor_nation_color();
	}

	return this->color;
}

const std::string &domain::get_name(const government_type *government_type, const domain_tier tier) const
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
			sub_find_iterator = find_iterator->second.find(domain_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	return this->get_name();
}

std::string domain::get_titled_name(const government_type *government_type, const domain_tier tier, const culture *culture, const religion *religion) const
{
	auto find_iterator = this->short_names.find(government_type);
	if (find_iterator == this->short_names.end()) {
		find_iterator = this->short_names.find(government_type->get_group());
	}

	if (find_iterator != this->short_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(domain_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	if (this->has_short_name()) {
		return this->get_name();
	}

	const std::string title_name = this->get_title_name(government_type, tier, culture, religion);
	const std::string country_name = this->get_name();
	if (this->definite_article) {
		return std::format("{} of the {}", title_name, country_name);
	} else {
		return std::format("{} of {}", title_name, country_name);
	}
}

const std::string &domain::get_title_name(const government_type *government_type, const domain_tier tier, const culture *culture, const religion *religion) const
{
	if (government_type == nullptr) {
		return domain_tier_data::get(tier)->get_name();
	}

	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator == this->title_names.end()) {
		find_iterator = this->title_names.find(government_type->get_group());
	}

	if (find_iterator != this->title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(domain_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	if (government_type->get_group()->is_religious()) {
		assert_throw(religion != nullptr);

		const std::string &religion_title_name = religion->get_title_name(government_type, tier);
		if (!religion_title_name.empty()) {
			return religion_title_name;
		}
	}

	assert_throw(culture != nullptr);
	const std::string &culture_title_name = culture->get_title_name(government_type, tier);
	if (!culture_title_name.empty()) {
		return culture_title_name;
	}

	assert_throw(government_type != nullptr);

	return government_type->get_title_name(tier);
}

const std::string &domain::get_office_title_name(const office *office, const government_type *government_type, const domain_tier tier, const gender gender, const culture *culture, const religion *religion) const
{
	const auto office_find_iterator = this->office_title_names.find(office);
	if (office_find_iterator != this->office_title_names.end()) {
		auto find_iterator = office_find_iterator->second.find(government_type);
		if (find_iterator == office_find_iterator->second.end()) {
			find_iterator = office_find_iterator->second.find(government_type->get_group());
		}

		if (find_iterator != office_find_iterator->second.end()) {
			auto sub_find_iterator = find_iterator->second.find(tier);
			if (sub_find_iterator == find_iterator->second.end()) {
				sub_find_iterator = find_iterator->second.find(domain_tier::none);
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
	}

	if (government_type->get_group()->is_religious() && office->is_ruler()) {
		assert_throw(religion != nullptr);
		const std::string &religion_office_title_name = religion->get_office_title_name(office, government_type, tier, gender);
		if (!religion_office_title_name.empty()) {
			return religion_office_title_name;
		}
	}

	assert_throw(culture != nullptr);
	const std::string &culture_office_title_name = culture->get_office_title_name(office, government_type, tier, gender);
	if (!culture_office_title_name.empty()) {
		return culture_office_title_name;
	}

	assert_throw(government_type != nullptr);

	return government_type->get_office_title_name(office, tier, gender);
}

bool domain::can_declare_war() const
{
	return this->get_type() == country_type::polity;
}

std::vector<const technology *> domain::get_available_technologies() const
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

QVariantList domain::get_available_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_technologies());
}

}
