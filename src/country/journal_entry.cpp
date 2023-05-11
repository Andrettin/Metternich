#include "metternich.h"

#include "country/journal_entry.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "infrastructure/building_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "util/string_util.h"

namespace metternich {

journal_entry::journal_entry(const std::string &identifier) : named_data_entry(identifier)
{
}

journal_entry::~journal_entry()
{
}

void journal_entry::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "preconditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->preconditions = std::move(conditions);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "completion_conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->completion_conditions = std::move(conditions);
	} else if (tag == "failure_conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->failure_conditions = std::move(conditions);
	} else if (tag == "completion_effects") {
		auto effects = std::make_unique<effect_list<const country>>();
		database::process_gsml_data(effects, scope);
		this->completion_effects = std::move(effects);
	} else if (tag == "failure_effects") {
		auto effects = std::make_unique<effect_list<const country>>();
		database::process_gsml_data(effects, scope);
		this->failure_effects = std::move(effects);
	} else if (tag == "active_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->active_modifier = std::move(modifier);
	} else if (tag == "completion_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->completion_modifier = std::move(modifier);
	} else if (tag == "owned_provinces") {
		for (const std::string &value : values) {
			this->owned_provinces.push_back(province::get(value));
		}
	} else if (tag == "owned_sites") {
		for (const std::string &value : values) {
			this->owned_sites.push_back(site::get(value));
		}
	} else if (tag == "built_buildings") {
		for (const std::string &value : values) {
			this->built_buildings.push_back(building_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void journal_entry::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Journal entry \"{}\" does not have a portrait.", this->get_identifier()));
	}

	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->completion_conditions != nullptr) {
		this->completion_conditions->check_validity();
	}

	if (this->get_failure_conditions() != nullptr) {
		this->get_failure_conditions()->check_validity();
	}

	if (this->get_completion_effects() != nullptr) {
		this->get_completion_effects()->check();
	}

	if (this->get_failure_effects() != nullptr) {
		this->get_failure_effects()->check();
	}
}

bool journal_entry::check_completion_conditions(const country *country) const
{
	if (this->completion_conditions == nullptr && this->owned_provinces.empty() && this->owned_sites.empty() && this->get_built_buildings().empty()) {
		//no completion conditions at all, so the entry can't be completed normally
		return false;
	}

	const read_only_context ctx(country);

	if (this->completion_conditions != nullptr && !this->completion_conditions->check(country, ctx)) {
		return false;
	}

	const country_game_data *country_game_data = country->get_game_data();

	for (const province *province : this->owned_provinces) {
		if (province->get_game_data()->get_owner() != country) {
			return false;
		}
	}

	for (const site *site : this->owned_sites) {
		if (!site->get_game_data()->is_on_map()) {
			return false;
		}

		if (site->get_game_data()->get_province() == nullptr) {
			return false;
		}

		if (site->get_game_data()->get_province()->get_game_data()->get_owner() != country) {
			return false;
		}
	}

	for (const building_type *building : this->get_built_buildings()) {
		if (!country_game_data->has_building(building)) {
			return false;
		}
	}

	return true;
}

QString journal_entry::get_completion_conditions_string() const
{
	std::string str;

	if (this->completion_conditions != nullptr) {
		str = this->completion_conditions->get_string(0);
	}

	for (const province *province : this->owned_provinces) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("Own {}", province->get_game_data()->get_current_cultural_name());
	}

	for (const site *site : this->owned_sites) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("Own {}", site->get_game_data()->get_current_cultural_name());
	}

	for (const building_type *building : this->get_built_buildings()) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("Build {} {}", string::get_indefinite_article(building->get_name()), building->get_name());
	}

	return QString::fromStdString(str);
}

QString journal_entry::get_failure_conditions_string() const
{
	if (this->get_failure_conditions() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_failure_conditions()->get_string(0));
}

QString journal_entry::get_completion_effects_string(metternich::country *country) const
{
	std::string str;

	if (this->get_completion_effects() != nullptr) {
		str = this->get_completion_effects()->get_effects_string(country, read_only_context(country));
	}

	if (this->get_completion_modifier() != nullptr) {
		std::string modifier_str = this->get_completion_modifier()->get_string();

		if (!modifier_str.empty()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::move(modifier_str);
		}
	}

	return QString::fromStdString(str);
}

QString journal_entry::get_failure_effects_string(metternich::country *country) const
{
	if (this->get_failure_effects() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_failure_effects()->get_effects_string(country, read_only_context(country)));
}

std::vector<const building_type *> journal_entry::get_built_buildings_with_requirements() const
{
	std::vector<const building_type *> buildings = this->get_built_buildings();

	for (size_t i = 0; i < buildings.size(); ++i) {
		const building_type *building = buildings[i];

		if (building->get_required_building() != nullptr) {
			buildings.push_back(building->get_required_building());
		}
	}

	return buildings;
}

}
