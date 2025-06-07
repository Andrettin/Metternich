#include "metternich.h"

#include "technology/technology.h"

#include "character/character.h"
#include "character/character_role.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "country/government_type.h"
#include "country/law.h"
#include "country/tradition.h"
#include "country/tradition_category.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "economy/resource.h"
#include "game/game.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/pathway.h"
#include "infrastructure/wonder.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "script/condition/and_condition.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "technology/technological_period.h"
#include "technology/technology_category.h"
#include "technology/technology_subcategory.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_type.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace metternich {
	
void technology::initialize_all()
{
	data_type::initialize_all();
	std::vector<technology *> costless_technologies = technology::get_all();
	std::erase_if(costless_technologies, [](const technology *technology) {
		return technology->is_discovery() || (technology->get_wealth_cost() != 0 && !technology->get_commodity_costs().empty());
	});

	bool changed = true;
	while (changed) {
		while (changed) {
			changed = false;

			for (technology *technology : costless_technologies) {
				if (technology->get_wealth_cost() != 0 && !technology->get_commodity_costs().empty()) {
					continue;
				}

				const bool success = technology->initialize_costs_from_prerequisites();
				if (success) {
					changed = true;
				}
			}

			std::erase_if(costless_technologies, [](const technology *technology) {
				return technology->get_wealth_cost() != 0 && !technology->get_commodity_costs().empty();
			});
		}

		changed = false;
		for (technology *technology : costless_technologies) {
			const bool success = technology->initialize_costs_from_dependents();
			if (success) {
				changed = true;
			}
		}

		std::erase_if(costless_technologies, [](const technology *technology) {
			return technology->get_wealth_cost() != 0 && !technology->get_commodity_costs().empty();
		});
	}

	technology::sort_instances([](const technology *lhs, const technology *rhs) {
		if (lhs->get_category() != rhs->get_category() && lhs->get_category() != nullptr && rhs->get_category() != nullptr) {
			return lhs->get_category()->get_identifier() < rhs->get_category()->get_identifier();
		}

		if (lhs->get_subcategory() != rhs->get_subcategory() && lhs->get_subcategory() != nullptr && rhs->get_subcategory() != nullptr) {
			return lhs->get_subcategory()->get_identifier() < rhs->get_subcategory()->get_identifier();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	commodity_map<int> research_commodity_counts;
	for (const technology *technology : technology::get_all()) {
		for (const auto &[commodity, cost] : technology->get_commodity_costs()) {
			research_commodity_counts[commodity] += cost;
		}
	}

	technology::research_commodities = archimedes::map::get_keys(research_commodity_counts);

	std::sort(technology::research_commodities.begin(), technology::research_commodities.end(), [&research_commodity_counts](const commodity *lhs, const commodity *rhs) {
		const int lhs_count = research_commodity_counts.find(lhs)->second;
		const int rhs_count = research_commodity_counts.find(rhs)->second;
		if (lhs_count != rhs_count) {
			return lhs_count > rhs_count;
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

technology::technology(const std::string &identifier)
	: named_data_entry(identifier)
{
}

technology::~technology()
{
}

void technology::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "cultures") {
		for (const std::string &value : values) {
			this->cultures.insert(culture::get(value));
		}
	} else if (tag == "cultural_groups") {
		for (const std::string &value : values) {
			this->cultural_groups.push_back(cultural_group::get(value));
		}
	} else if (tag == "prerequisites") {
		for (const std::string &value : values) {
			this->prerequisites.push_back(technology::get(value));
		}
	} else if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "cost_factor") {
		auto factor = std::make_unique<metternich::factor<country>>(100);
		database::process_gsml_data(factor, scope);
		this->cost_factor = std::move(factor);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void technology::initialize()
{
	if (this->subcategory != nullptr) {
		this->subcategory->add_technology(this);
	}

	this->calculate_total_prerequisite_depth();

	for (technology *prerequisite : this->get_prerequisites()) {
		prerequisite->leads_to.push_back(this);
	}

	std::sort(this->enabled_pathways.begin(), this->enabled_pathways.end(), pathway_compare());
	std::sort(this->enabled_river_crossing_pathways.begin(), this->enabled_river_crossing_pathways.end(), pathway_compare());

	named_data_entry::initialize();
}

void technology::check() const
{
	if (this->get_subcategory() == nullptr) {
		throw std::runtime_error(std::format("Technology \"{}\" has no subcategory.", this->get_identifier()));
	}

	if (this->get_category() == nullptr) {
		throw std::runtime_error(std::format("Technology \"{}\" has no category.", this->get_identifier()));
	}

	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Technology \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Technology \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_wealth_cost() == 0 && this->get_commodity_costs().empty() && !this->is_discovery()) {
		//throw std::runtime_error(std::format("Technology \"{}\" has no cost, and is not a discovery.", this->get_identifier()));
	}

	if (this->get_period() != nullptr) {
		if (this->get_year() != 0) {
			if (this->get_year() < this->get_period()->get_start_year() || this->get_year() > this->get_period()->get_end_year()) {
				log::log_error(std::format("The year for technology \"{}\" ({}) does not match its period (\"{}\", {}-{}).", this->get_identifier(), this->get_year(), this->get_period()->get_identifier(), this->get_period()->get_start_year(), this->get_period()->get_end_year(), this->get_period()->get_index()));
			}
		}

		if (this->get_period()->get_index() != this->get_total_prerequisite_depth()) {
			log::log_error(std::format("The period for technology \"{}\" (\"{}\", {}-{}, index {}) does not match its total prerequisite depth ({}).", this->get_identifier(), this->get_period()->get_identifier(), this->get_period()->get_start_year(), this->get_period()->get_end_year(), this->get_period()->get_index(), this->get_total_prerequisite_depth()));
		}

		if (this->leads_to.empty() && !this->get_period()->get_child_periods().empty()) {
			log::log_error(std::format("Technology \"{}\" is a dead end technology, but its period has child periods.", this->get_identifier()));
		}
	} else {
		log::log_error(std::format("Technology \"{}\" has no period.", this->get_identifier()));
	}

	if (
		this->get_modifier() == nullptr
		&& !this->grants_free_technology()
		&& this->get_shared_prestige() == 0
		&& this->get_enabled_buildings().empty()
		&& this->get_enabled_characters(character_role::ruler).empty()
		&& this->get_enabled_characters(character_role::advisor).empty()
		&& this->get_enabled_characters(character_role::governor).empty()
		&& this->get_enabled_characters(character_role::landholder).empty()
		&& this->get_enabled_characters(character_role::leader).empty()
		&& this->get_enabled_civilian_units().empty()
		&& this->get_enabled_commodities().empty()
		&& this->get_enabled_improvements().empty()
		&& this->get_enabled_laws().empty()
		&& this->get_enabled_traditions().empty()
		&& this->get_enabled_military_units().empty()
		&& this->get_enabled_pathway_terrains().empty()
		&& this->get_enabled_pathways().empty()
		&& this->get_enabled_production_types().empty()
		&& this->get_enabled_resources().empty()
		&& this->get_enabled_river_crossing_pathways().empty()
		&& this->get_enabled_transporters().empty()
		&& this->get_enabled_wonders().empty()
	) {
		log::log_error(std::format("Technology \"{}\" has no effects.", this->get_identifier()));
	}
}

const technology_category *technology::get_category() const
{
	if (this->get_subcategory() != nullptr) {
		return this->get_subcategory()->get_category();
	}

	return nullptr;
}

bool technology::is_available_for_country(const country *country) const
{
	if (!this->cultures.empty() || !this->cultural_groups.empty()) {
		if (this->cultures.contains(country->get_culture())) {
			return true;
		}

		for (const cultural_group *cultural_group : this->cultural_groups) {
			if (country->get_culture()->is_part_of_group(cultural_group)) {
				return true;
			}
		}

		return false;
	}

	return true;
}

int technology::get_shared_prestige_for_country(const country *country) const
{
	int prestige = this->get_shared_prestige();

	if (prestige <= 1) {
		return prestige;
	}

	for (const metternich::country *loop_country : game::get()->get_countries()) {
		if (loop_country == country) {
			continue;
		}

		if (loop_country->get_game_data()->has_technology(this)) {
			prestige /= 2;
		}

		prestige = std::max(prestige, 1);

		if (prestige == 1) {
			break;
		}
	}

	return prestige;
}

QVariantList technology::get_prerequisites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_prerequisites());
}

bool technology::requires_technology(const technology *technology) const
{
	assert_throw(this != technology);

	for (const metternich::technology *prerequisite : this->get_prerequisites()) {
		if (prerequisite == technology || prerequisite->requires_technology(technology)) {
			return true;
		}
	}

	return false;
}

void technology::calculate_total_prerequisite_depth()
{
	if (this->total_prerequisite_depth != 0 || this->get_prerequisites().empty()) {
		return;
	}

	int depth = 0;

	for (technology *prerequisite : this->get_prerequisites()) {
		prerequisite->calculate_total_prerequisite_depth();
		depth = std::max(depth, prerequisite->get_total_prerequisite_depth() + 1);
	}

	this->total_prerequisite_depth = depth;
}

int technology::get_wealth_cost_for_country(const country *country) const
{
	centesimal_int cost(this->get_wealth_cost());

	if (cost > 0) {
		cost *= country->get_game_data()->get_research_cost_modifier();
		cost /= 100;

		cost *= centesimal_int(100) + country->get_game_data()->get_technology_cost_modifier() + country->get_game_data()->get_technology_category_cost_modifier(this->get_category()) + country->get_game_data()->get_technology_subcategory_cost_modifier(this->get_subcategory());
		cost /= 100;

		if (this->get_cost_factor() != nullptr) {
			cost = this->get_cost_factor()->calculate(country, cost);
		}

		cost = centesimal_int::max(centesimal_int(1), cost);
	}

	return cost.to_int();
}

commodity_map<int> technology::get_commodity_costs_for_country(const country *country) const
{
	commodity_map<int> costs = this->get_commodity_costs();

	for (auto &[commodity, cost_int] : costs) {
		if (cost_int > 0) {
			centesimal_int cost(cost_int);

			cost *= country->get_game_data()->get_research_cost_modifier();
			cost /= 100;

			cost *= centesimal_int(100) + country->get_game_data()->get_technology_cost_modifier() + country->get_game_data()->get_technology_category_cost_modifier(this->get_category()) + country->get_game_data()->get_technology_subcategory_cost_modifier(this->get_subcategory());
			cost /= 100;

			if (this->get_cost_factor() != nullptr) {
				cost = this->get_cost_factor()->calculate(country, cost);
			}

			cost_int = std::max(1, cost.to_int());
		}
	}

	return costs;
}

QVariantList technology::get_commodity_costs_for_country_qvariant_list(const country *country) const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_costs_for_country(country));
}

bool technology::initialize_costs_from_prerequisites()
{
	bool changed = false;

	if (this->get_wealth_cost() == 0) {
		int highest_prerequisite_wealth_cost = 0;
		int wealth_cost_modifier = 0;

		for (const technology *prerequisite : this->get_prerequisites()) {
			if (prerequisite->get_total_prerequisite_depth() != (this->get_total_prerequisite_depth() - 1)) {
				continue;
			}

			if (prerequisite->get_wealth_cost() == 0) {
				continue;
			}

			highest_prerequisite_wealth_cost = std::max(prerequisite->get_wealth_cost(), highest_prerequisite_wealth_cost);

			int prerequisite_wealth_cost_modifier = 0;
			for (const technology *subprerequisite : prerequisite->get_prerequisites()) {
				if (subprerequisite->get_total_prerequisite_depth() != (prerequisite->get_total_prerequisite_depth() - 1)) {
					continue;
				}

				if (subprerequisite->get_wealth_cost() == 0) {
					continue;
				}

				if (subprerequisite->get_wealth_cost() > prerequisite->get_wealth_cost()) {
					continue;
				}

				prerequisite_wealth_cost_modifier = std::max(prerequisite->get_wealth_cost() * 100 / subprerequisite->get_wealth_cost(), prerequisite_wealth_cost_modifier);
			}

			wealth_cost_modifier = std::max(prerequisite_wealth_cost_modifier, wealth_cost_modifier);
		}

		if (wealth_cost_modifier != 0) {
			this->wealth_cost = std::max(1, highest_prerequisite_wealth_cost * wealth_cost_modifier / 100);
			changed = true;
		}
	}

	if (this->get_commodity_costs().empty()) {
		commodity_map<int> highest_prerequisite_commodity_costs;
		commodity_map<int> commodity_cost_modifiers;

		for (const technology *prerequisite : this->get_prerequisites()) {
			if (prerequisite->get_total_prerequisite_depth() != (this->get_total_prerequisite_depth() - 1)) {
				continue;
			}

			if (prerequisite->get_commodity_costs().empty()) {
				continue;
			}

			for (const auto &[commodity, cost] : prerequisite->get_commodity_costs()) {
				highest_prerequisite_commodity_costs[commodity] = std::max(cost, highest_prerequisite_commodity_costs[commodity]);
			}

			commodity_map<int> prerequisite_commodity_cost_modifiers;
			for (const technology *subprerequisite : prerequisite->get_prerequisites()) {
				if (subprerequisite->get_total_prerequisite_depth() != (prerequisite->get_total_prerequisite_depth() - 1)) {
					continue;
				}

				if (subprerequisite->get_commodity_costs().empty()) {
					continue;
				}

				if (subprerequisite->get_total_commodity_cost() > prerequisite->get_total_commodity_cost()) {
					continue;
				}

				if (archimedes::map::get_keys(prerequisite->get_commodity_costs()) != archimedes::map::get_keys(subprerequisite->get_commodity_costs())) {
					continue;
				}

				for (const auto &[commodity, cost] : prerequisite->get_commodity_costs()) {
					const int subprerequisite_cost = subprerequisite->get_commodity_costs().find(commodity)->second;
					prerequisite_commodity_cost_modifiers[commodity] = std::max(cost * 100 / subprerequisite_cost, prerequisite_commodity_cost_modifiers[commodity]);
				}
			}

			for (const auto &[commodity, modifier] : prerequisite_commodity_cost_modifiers) {
				commodity_cost_modifiers[commodity] = std::max(modifier, commodity_cost_modifiers[commodity]);
			}
		}

		if (!commodity_cost_modifiers.empty()) {
			for (const auto &[commodity, modifier] : commodity_cost_modifiers) {
				assert_throw(modifier >= 100);
				this->commodity_costs[commodity] = std::max(1, highest_prerequisite_commodity_costs[commodity] * modifier / 100);
			}

			changed = true;
		}
	}

	return changed;
}

bool technology::initialize_costs_from_dependents()
{
	bool changed = false;

	if (this->get_wealth_cost() == 0) {
		int highest_dependent_wealth_cost = 0;
		int wealth_cost_modifier = 0;

		for (const technology *dependent : this->get_leads_to()) {
			if (dependent->get_total_prerequisite_depth() != (this->get_total_prerequisite_depth() + 1)) {
				continue;
			}

			if (dependent->get_wealth_cost() == 0) {
				continue;
			}

			highest_dependent_wealth_cost = std::max(dependent->get_wealth_cost(), highest_dependent_wealth_cost);

			int dependent_wealth_cost_modifier = 0;
			for (const technology *subdependent : dependent->get_leads_to()) {
				if (subdependent->get_total_prerequisite_depth() != (dependent->get_total_prerequisite_depth() + 1)) {
					continue;
				}

				if (subdependent->get_wealth_cost() == 0) {
					continue;
				}

				if (subdependent->get_wealth_cost() < dependent->get_wealth_cost()) {
					continue;
				}

				dependent_wealth_cost_modifier = std::max(dependent->get_wealth_cost() * 100 / subdependent->get_wealth_cost(), dependent_wealth_cost_modifier);
			}

			wealth_cost_modifier = std::max(dependent_wealth_cost_modifier, wealth_cost_modifier);
		}

		if (wealth_cost_modifier != 0) {
			this->wealth_cost = std::max(1, highest_dependent_wealth_cost * wealth_cost_modifier / 100);
			changed = true;
		}
	}

	if (this->get_commodity_costs().empty()) {
		commodity_map<int> highest_dependent_commodity_costs;
		commodity_map<int> commodity_cost_modifiers;

		for (const technology *dependent : this->get_leads_to()) {
			if (dependent->get_total_prerequisite_depth() != (this->get_total_prerequisite_depth() + 1)) {
				continue;
			}

			if (dependent->get_commodity_costs().empty()) {
				continue;
			}

			for (const auto &[commodity, cost] : dependent->get_commodity_costs()) {
				highest_dependent_commodity_costs[commodity] = std::max(cost, highest_dependent_commodity_costs[commodity]);
			}

			commodity_map<int> dependent_commodity_cost_modifiers;
			for (const technology *subdependent : dependent->get_leads_to()) {
				if (subdependent->get_total_prerequisite_depth() != (dependent->get_total_prerequisite_depth() + 1)) {
					continue;
				}

				if (subdependent->get_commodity_costs().empty()) {
					continue;
				}

				if (subdependent->get_total_commodity_cost() < dependent->get_total_commodity_cost()) {
					continue;
				}

				if (archimedes::map::get_keys(dependent->get_commodity_costs()) != archimedes::map::get_keys(subdependent->get_commodity_costs())) {
					continue;
				}

				for (const auto &[commodity, cost] : dependent->get_commodity_costs()) {
					dependent_commodity_cost_modifiers[commodity] = std::max(cost * 100 / subdependent->get_commodity_costs().find(commodity)->second, dependent_commodity_cost_modifiers[commodity]);
				}
			}

			for (const auto &[commodity, modifier] : dependent_commodity_cost_modifiers) {
				commodity_cost_modifiers[commodity] = std::max(modifier, commodity_cost_modifiers[commodity]);
			}
		}

		if (!commodity_cost_modifiers.empty()) {
			for (const auto &[commodity, modifier] : commodity_cost_modifiers) {
				assert_throw(modifier <= 100);
				this->commodity_costs[commodity] = std::max(1, highest_dependent_commodity_costs[commodity] * modifier / 100);
			}

			changed = true;
		}
	}

	return changed;
}

QVariantList technology::get_enabled_buildings_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_buildings());
}

std::vector<const building_type *> technology::get_enabled_buildings_for_culture(const culture *culture) const
{
	std::vector<const building_type *> buildings;

	for (const building_type *building : this->get_enabled_buildings()) {
		if (building != culture->get_building_class_type(building->get_building_class())) {
			continue;
		}

		buildings.push_back(building);
	}

	return buildings;
}

std::vector<const wonder *> technology::get_enabled_wonders_for_country(const country *country) const
{
	std::vector<const wonder *> wonders;

	for (const wonder *wonder : this->get_enabled_wonders()) {
		if (wonder->get_conditions() != nullptr && !wonder->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		wonders.push_back(wonder);
	}

	return wonders;
}

std::vector<const wonder *> technology::get_disabled_wonders_for_country(const country *country) const
{
	std::vector<const wonder *> wonders;

	for (const wonder *wonder : this->get_disabled_wonders()) {
		if (wonder->get_conditions() != nullptr && !wonder->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		wonders.push_back(wonder);
	}

	return wonders;
}

QVariantList technology::get_enabled_improvements_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_improvements());
}

QVariantList technology::get_enabled_pathways_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_pathways());
}

QVariantList technology::get_enabled_civilian_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_civilian_units());
}

std::vector<const civilian_unit_type *> technology::get_enabled_civilian_units_for_culture(const culture *culture) const
{
	std::vector<const civilian_unit_type *> civilian_units;

	for (const civilian_unit_type *civilian_unit : this->get_enabled_civilian_units()) {
		assert_throw(civilian_unit->get_unit_class() != nullptr);

		if (civilian_unit != culture->get_civilian_class_unit_type(civilian_unit->get_unit_class())) {
			continue;
		}

		civilian_units.push_back(civilian_unit);
	}

	return civilian_units;
}

void technology::add_enabled_civilian_unit(const civilian_unit_type *civilian_unit)
{
	this->enabled_civilian_units.push_back(civilian_unit);

	std::sort(this->enabled_civilian_units.begin(), this->enabled_civilian_units.end(), [](const civilian_unit_type *lhs, const civilian_unit_type *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});
}

QVariantList technology::get_enabled_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_military_units());
}

std::vector<const military_unit_type *> technology::get_enabled_military_units_for_culture(const culture *culture) const
{
	std::vector<const military_unit_type *> military_units;

	for (const military_unit_type *military_unit : this->get_enabled_military_units()) {
		assert_throw(military_unit->get_unit_class() != nullptr);

		if (military_unit != culture->get_military_class_unit_type(military_unit->get_unit_class())) {
			continue;
		}

		military_units.push_back(military_unit);
	}

	return military_units;
}

void technology::add_enabled_military_unit(const military_unit_type *military_unit)
{
	this->enabled_military_units.push_back(military_unit);

	std::sort(this->enabled_military_units.begin(), this->enabled_military_units.end(), [](const military_unit_type *lhs, const military_unit_type *rhs) {
		if (lhs->get_category() != rhs->get_category()) {
			return lhs->get_category() < rhs->get_category();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

QVariantList technology::get_enabled_transporters_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_transporters());
}

std::vector<const transporter_type *> technology::get_enabled_transporters_for_culture(const culture *culture) const
{
	std::vector<const transporter_type *> transporters;

	for (const transporter_type *transporter : this->get_enabled_transporters()) {
		assert_throw(transporter->get_transporter_class() != nullptr);

		if (transporter != culture->get_transporter_class_type(transporter->get_transporter_class())) {
			continue;
		}

		transporters.push_back(transporter);
	}

	return transporters;
}

void technology::add_enabled_transporter(const transporter_type *transporter)
{
	this->enabled_transporters.push_back(transporter);

	std::sort(this->enabled_transporters.begin(), this->enabled_transporters.end(), [](const transporter_type *lhs, const transporter_type *rhs) {
		if (lhs->get_category() != rhs->get_category()) {
			return lhs->get_category() < rhs->get_category();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

void technology::add_enabled_law(const law *law)
{
	this->enabled_laws.push_back(law);

	std::sort(this->enabled_laws.begin(), this->enabled_laws.end(), [](const metternich::law *lhs, const metternich::law *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});
}

std::vector<const tradition *> technology::get_enabled_traditions_for_country(const country *country) const
{
	std::vector<const tradition *> traditions;

	for (const tradition *tradition : this->get_enabled_traditions()) {
		if (!tradition->is_available_for_country(country)) {
			continue;
		}

		traditions.push_back(tradition);
	}

	return traditions;
}

void technology::add_enabled_tradition(const tradition *tradition)
{
	this->enabled_traditions.push_back(tradition);

	std::sort(this->enabled_traditions.begin(), this->enabled_traditions.end(), [](const metternich::tradition *lhs, const metternich::tradition *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});
}

std::vector<const character *> technology::get_enabled_characters_for_country(const character_role role, const country *country) const
{
	std::vector<const character *> characters;

	for (const character *character : this->get_enabled_characters(role)) {
		if (role == character_role::ruler && !vector::contains(country->get_rulers(), character)) {
			continue;
		}

		if (role == character_role::governor && !vector::contains(country->get_game_data()->get_provinces(), character->get_governable_province())) {
			continue;
		}

		if (role == character_role::landholder && character->get_holdable_site()->get_game_data()->get_owner() != country) {
			continue;
		}

		if (character->get_conditions() != nullptr && !character->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		characters.push_back(character);
	}

	return characters;
}

void technology::add_enabled_character(const character_role role, const character *character)
{
	this->enabled_characters[role].push_back(character);

	std::sort(this->enabled_characters[role].begin(), this->enabled_characters[role].end(), [](const metternich::character *lhs, const metternich::character *rhs) {
		return lhs->get_full_name() < rhs->get_full_name();
	});
}

std::vector<const character *> technology::get_retired_characters_for_country(const character_role role, const country *country) const
{
	std::vector<const character *> characters;

	for (const character *character : this->get_retired_characters(role)) {
		if (role == character_role::ruler && !vector::contains(country->get_rulers(), character)) {
			continue;
		}

		if (role == character_role::governor && !vector::contains(country->get_game_data()->get_provinces(), character->get_governable_province())) {
			continue;
		}

		if (role == character_role::landholder && character->get_holdable_site()->get_game_data()->get_owner() != country) {
			continue;
		}

		if (character->get_conditions() != nullptr && !character->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		characters.push_back(character);
	}

	return characters;
}

void technology::add_retired_character(const character_role role, const character *character)
{
	this->retired_characters[role].push_back(character);

	std::sort(this->retired_characters[role].begin(), this->retired_characters[role].end(), [](const metternich::character *lhs, const metternich::character *rhs) {
		return lhs->get_full_name() < rhs->get_full_name();
	});
}

std::string technology::get_modifier_string(const country *country) const
{
	if (this->get_modifier() == nullptr) {
		return std::string();
	}

	return this->get_modifier()->get_string(country);
}

QString technology::get_effects_string(metternich::country *country) const
{
	std::string str = this->get_modifier_string(country);

	if (this->grants_free_technology()) {
		if (!str.empty()) {
			str += "\n";
		}

		str += "Free technology for the first to research";
	}

	if (this->get_shared_prestige() > 0) {
		if (!str.empty()) {
			str += "\n";
		}

		const int prestige = this->get_shared_prestige_for_country(country);
		str += std::format("+{} {}", prestige, defines::get()->get_prestige_commodity()->get_name());
	}

	if (!this->get_enabled_commodities().empty()) {
		for (const commodity *commodity : this->get_enabled_commodities()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} commodity", commodity->get_name());
		}
	}

	if (!this->get_enabled_resources().empty()) {
		for (const resource *resource : this->get_enabled_resources()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} resource", resource->get_name());
		}
	}

	const std::vector<const building_type *> buildings = this->get_enabled_buildings_for_culture(country->get_culture());
	if (!buildings.empty()) {
		for (const building_type *building : buildings) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} building", building->get_name());
		}
	}

	const std::vector<const wonder *> enabled_wonders = this->get_enabled_wonders_for_country(country);
	if (!enabled_wonders.empty()) {
		for (const wonder *wonder : enabled_wonders) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} wonder", wonder->get_name());
		}
	}

	const std::vector<const wonder *> disabled_wonders = this->get_disabled_wonders_for_country(country);
	if (!disabled_wonders.empty()) {
		for (const wonder *wonder : disabled_wonders) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Obsoletes {} wonder", wonder->get_name());
		}
	}

	if (!this->get_enabled_production_types().empty()) {
		for (const production_type *production_type : this->get_enabled_production_types()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} production type", production_type->get_name());
		}
	}

	if (!this->get_enabled_improvements().empty()) {
		for (const improvement *improvement : this->get_enabled_improvements()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} improvement", improvement->get_name());
		}
	}

	if (!this->get_enabled_pathways().empty()) {
		for (const pathway *pathway : this->get_enabled_pathways()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {}", pathway->get_name());
		}
	}

	if (!this->get_enabled_river_crossing_pathways().empty()) {
		for (const pathway *pathway : this->get_enabled_river_crossing_pathways()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} across rivers", pathway->get_name());
		}
	}

	if (!this->get_enabled_pathway_terrains().empty()) {
		for (const auto &[pathway, terrains] : this->get_enabled_pathway_terrains()) {
			for (const terrain_type *terrain : terrains) {
				if (!str.empty()) {
					str += "\n";
				}

				str += std::format("Enables {} in {}", pathway->get_name(), terrain->get_name());
			}
		}
	}

	const std::vector<const civilian_unit_type *> civilian_units = this->get_enabled_civilian_units_for_culture(country->get_culture());
	if (!civilian_units.empty()) {
		for (const civilian_unit_type *civilian_unit : civilian_units) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} civilian unit", civilian_unit->get_name());
		}
	}

	const std::vector<const military_unit_type *> military_units = get_enabled_military_units_for_culture(country->get_culture());
	if (!military_units.empty()) {
		for (const military_unit_type *military_unit : military_units) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} {}", military_unit->get_name(), military_unit->get_domain() == military_unit_domain::water ? "warship" : "regiment");
		}
	}

	const std::vector<const transporter_type *> transporters = get_enabled_transporters_for_culture(country->get_culture());
	if (!transporters.empty()) {
		for (const transporter_type *transporter : transporters) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} {}", transporter->get_name(), transporter->is_ship() ? "merchant ship" : "transporter");
		}
	}

	if (!this->get_enabled_laws().empty()) {
		for (const law *law : this->get_enabled_laws()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} law", law->get_name());
		}
	}

	const std::vector<const tradition *> enabled_traditions = this->get_enabled_traditions_for_country(country);
	if (!enabled_traditions.empty()) {
		for (const tradition *tradition : enabled_traditions) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} {}", tradition->get_name(), string::lowered(get_tradition_category_name(tradition->get_category())));
		}
	}

	this->write_character_effects_string(character_role::ruler, "ruler", country, str);
	this->write_character_effects_string(character_role::advisor, "advisor", country, str);
	this->write_character_effects_string(character_role::governor, "governor", country, str);
	this->write_character_effects_string(character_role::landholder, "landholder", country, str);
	this->write_character_effects_string(character_role::leader, "leader", country, str);
	this->write_character_effects_string(character_role::civilian, "civilian", country, str);

	return QString::fromStdString(str);
}

void technology::write_character_effects_string(const character_role role, const std::string_view &role_name, const country *country, std::string &str) const
{
	const std::vector<const character *> enabled_characters = get_enabled_characters_for_country(role, country);

	if (!enabled_characters.empty()) {
		for (const character *character : enabled_characters) {
			if (!str.empty()) {
				str += "\n";
			}

			std::string character_type_name = std::string(role_name);

			switch (role) {
				case character_role::leader:
					character_type_name = string::lowered(character->get_leader_type_name());
					break;
				case character_role::civilian:
					character_type_name = string::lowered(character->get_civilian_unit_type()->get_name());
					break;
			}

			str += std::format("Enables {} {}", character->get_full_name(), character_type_name);
		}
	}

	const std::vector<const character *> retired_characters = get_retired_characters_for_country(role, country);

	if (!retired_characters.empty()) {
		for (const character *character : retired_characters) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Retires {} {}", character->get_full_name(), role_name);
		}
	}
}

bool technology::is_hidden_in_tree() const
{
	return !this->is_available_for_country(game::get()->get_player_country());
}

}
