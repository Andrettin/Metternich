#include "metternich.h"

#include "country/journal_entry.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/tradition.h"
#include "country/tradition_category.h"
#include "infrastructure/building_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "unit/civilian_unit_type.h"
#include "util/assert_util.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/vector_util.h"

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
	} else if (tag == "built_settlement_buildings") {
		scope.for_each_element([&](const gsml_property &property) {
			const site *settlement = site::get(property.get_key());
			const building_type *building = building_type::get(property.get_value());
			this->built_settlement_buildings[settlement].push_back(building);
		}, [&](const gsml_data &child_scope) {
			const site *settlement = site::get(child_scope.get_tag());

			for (const std::string &value : child_scope.get_values()) {
				this->built_settlement_buildings[settlement].push_back(building_type::get(value));
			}
		});
	} else if (tag == "researched_technologies") {
		for (const std::string &value : values) {
			this->researched_technologies.push_back(technology::get(value));
		}
	} else if (tag == "adopted_traditions") {
		for (const std::string &value : values) {
			this->adopted_traditions.push_back(tradition::get(value));
		}
	} else if (tag == "recruited_characters") {
		for (const std::string &value : values) {
			this->recruited_characters.push_back(character::get(value));
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

	if (this->preconditions != nullptr) {
		this->preconditions->check_validity();
	}

	if (this->conditions != nullptr) {
		this->conditions->check_validity();
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

	for (const auto &[settlement, buildings] : this->get_built_settlement_buildings()) {
		if (!settlement->is_settlement()) {
			throw std::runtime_error(std::format("Journal entry \"{}\" requires constructing a building in \"{}\", but that site is not a settlement.", this->get_identifier(), settlement->get_identifier()));
		}
	}

	for (const character *character : this->get_recruited_characters()) {
		switch (character->get_role()) {
			case character_role::advisor:
			case character_role::leader:
			case character_role::civilian:
				break;
			default:
				throw std::runtime_error(std::format("Journal entry \"{}\" requires the recruiting \"{}\" character, but that character does not have a recruitable role.", this->get_identifier(), character->get_identifier()));
		}
	}
}

bool journal_entry::check_preconditions(const country *country) const
{
	const read_only_context ctx(country);

	if (this->preconditions != nullptr && !this->preconditions->check(country, ctx)) {
		return false;
	}

	for (const character *character : this->get_recruited_characters()) {
		const metternich::country *character_country = character->get_game_data()->get_country();
		if (character_country != nullptr && character_country != country) {
			return false;
		}
	}

	return true;
}

bool journal_entry::check_conditions(const country *country) const
{
	const read_only_context ctx(country);

	if (this->conditions != nullptr && !this->conditions->check(country, ctx)) {
		return false;
	}

	const country_game_data *country_game_data = country->get_game_data();

	for (const technology *technology : this->get_researched_technologies()) {
		if (!technology->is_discovery() && !country_game_data->is_technology_available(technology) && !country_game_data->has_technology(technology)) {
			return false;
		}
	}

	for (const tradition *tradition : this->get_adopted_traditions()) {
		if (!country_game_data->can_have_tradition(tradition)) {
			return false;
		}
	}

	return true;
}

bool journal_entry::check_completion_conditions(const country *country, const bool ignore_random_chance) const
{
	if (this->completion_conditions == nullptr && this->owned_provinces.empty() && this->owned_sites.empty() && this->get_built_buildings().empty() && this->get_built_settlement_buildings().empty() && this->get_researched_technologies().empty() && this->get_adopted_traditions().empty() && this->get_recruited_characters().empty() && this->get_completion_random_chance() == 0) {
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

	for (const auto &[settlement, buildings] : this->get_built_settlement_buildings()) {
		assert_throw(settlement->is_settlement());

		for (const building_type *building : buildings) {
			if (!settlement->get_game_data()->has_building_or_better(building)) {
				return false;
			}
		}
	}

	for (const technology *technology : this->get_researched_technologies()) {
		if (!country_game_data->has_technology(technology)) {
			return false;
		}
	}

	for (const tradition *tradition : this->get_adopted_traditions()) {
		if (!country_game_data->has_tradition(tradition)) {
			return false;
		}
	}

	for (const character *character : this->get_recruited_characters()) {
		switch (character->get_role()) {
			case character_role::advisor:
				if (!vector::contains(country_game_data->get_advisors(), character)) {
					return false;
				}
				break;
			case character_role::leader:
				if (!vector::contains(country_game_data->get_leaders(), character)) {
					return false;
				}
				break;
			case character_role::civilian:
				if (!country_game_data->has_civilian_character(character)) {
					return false;
				}
				break;
			default:
				break;
		}
	}

	if (this->get_completion_random_chance() != 0 && !ignore_random_chance) {
		const int64_t random_number = random::get()->generate(decimillesimal_int::divisor * 100);
		if (this->get_completion_random_chance().get_value() <= random_number) {
			return false;
		}
	}

	return true;
}

QString journal_entry::get_completion_conditions_string() const
{
	std::string str;

	if (this->completion_conditions != nullptr) {
		str = this->completion_conditions->get_conditions_string(0);
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

	for (const auto &[settlement, buildings] : this->get_built_settlement_buildings()) {
		assert_throw(settlement->is_settlement());

		for (const building_type *building : buildings) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Build {} {} in {}", string::get_indefinite_article(building->get_name()), building->get_name(), settlement->get_game_data()->get_current_cultural_name());
		}
	}

	for (const technology *technology : this->get_researched_technologies()) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("Research {}", technology->get_name());
	}

	for (const tradition *tradition : this->get_adopted_traditions()) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("Adopt {} ({})", tradition->get_name(), get_tradition_category_name(tradition->get_category()));
	}

	for (const character *character : this->get_recruited_characters()) {
		if (!str.empty()) {
			str += "\n";
		}

		std::string character_type_name;

		switch (character->get_role()) {
			case character_role::advisor:
				character_type_name = "Advisor";
				break;
			case character_role::leader:
				character_type_name = character->get_leader_type_name();
				break;
			case character_role::civilian:
				character_type_name = character->get_civilian_unit_type()->get_name();
				break;
			default:
				break;
		}

		str += std::format("Recruit {} ({})", character->get_full_name(), character_type_name);
	}

	if (this->get_completion_random_chance() != 0) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("{}% chance", this->get_completion_random_chance().to_string());
	}

	return QString::fromStdString(str);
}

QString journal_entry::get_failure_conditions_string() const
{
	if (this->get_failure_conditions() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_failure_conditions()->get_conditions_string(0));
}

QString journal_entry::get_completion_effects_string(metternich::country *country) const
{
	std::string str;

	if (this->get_completion_effects() != nullptr) {
		str = this->get_completion_effects()->get_effects_string(country, read_only_context(country));
	}

	if (this->get_completion_modifier() != nullptr) {
		std::string modifier_str = this->get_completion_modifier()->get_string(country);

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

site_map<std::vector<const building_type *>> journal_entry::get_built_settlement_buildings_with_requirements() const
{
	site_map<std::vector<const building_type *>> settlement_buildings = this->get_built_settlement_buildings();

	for (auto &[settlement, buildings] : settlement_buildings) {
		for (size_t i = 0; i < buildings.size(); ++i) {
			const building_type *building = buildings[i];

			if (building->get_required_building() != nullptr) {
				buildings.push_back(building->get_required_building());
			}
		}
	}

	return settlement_buildings;
}

}
