#include "metternich.h"

#include "domain/country_technology.h"

#include "database/defines.h"
#include "domain/country_ai.h"
#include "domain/country_economy.h"
#include "domain/country_government.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/idea_slot.h"
#include "domain/idea_type.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/game.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/tile.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/point_container.h"
#include "util/map_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace metternich {

country_technology::country_technology(const metternich::domain *domain, const domain_game_data *game_data)
	: domain(domain)
{
	connect(game_data, &domain_game_data::available_idea_slots_changed, this, &country_technology::available_research_slots_changed);
}

country_technology::~country_technology()
{
}

domain_game_data *country_technology::get_game_data() const
{
	return this->domain->get_game_data();
}

void country_technology::do_research()
{
	try {
		if (this->get_gain_technologies_known_by_others_count() > 0) {
			this->gain_technologies_known_by_others();
		}

		assert_throw(this->free_technology_count >= 0);

		if (this->free_technology_count > 0) {
			this->gain_free_technology();
		}

		const technology_set current_researches = this->get_current_researches();
		for (const technology *current_research : current_researches) {
			this->on_technology_researched(current_research);
		}

		this->current_researches.clear();
		emit current_researches_changed();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing research for domain \"" + this->domain->get_identifier() + "\"."));
	}
}

const technology_set &country_technology::get_technologies() const
{
	if (this->get_game_data()->get_capital_province() != nullptr) {
		return this->get_game_data()->get_capital_province()->get_game_data()->get_technologies();
	} else if (this->domain->get_default_capital()->get_province()->get_game_data()->is_on_map()) {
		return this->domain->get_default_capital()->get_province()->get_game_data()->get_technologies();
	}

	static const technology_set empty_set;
	return empty_set;
}

QVariantList country_technology::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

void country_technology::add_technology(const technology *technology)
{
	if (this->get_game_data()->get_capital_province() != nullptr) {
		this->get_game_data()->get_capital_province()->get_game_data()->add_technology(technology);
	} else if (this->domain->get_default_capital()->get_province()->get_game_data()->is_on_map()) {
		this->domain->get_default_capital()->get_province()->get_game_data()->add_technology(technology);
	}
}

void country_technology::add_technology_with_prerequisites(const technology *technology)
{
	this->add_technology(technology);

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		this->add_technology_with_prerequisites(prerequisite);
	}
}

void country_technology::on_technology_added(const technology *technology)
{
	if (technology->get_modifier() != nullptr) {
		technology->get_modifier()->apply(this->domain, 1);
	}

	for (const commodity *enabled_commodity : technology->get_enabled_commodities()) {
		if (!enabled_commodity->is_enabled()) {
			continue;
		}

		this->domain->get_economy()->add_available_commodity(enabled_commodity);

		if (enabled_commodity->is_tradeable()) {
			this->domain->get_economy()->add_tradeable_commodity(enabled_commodity);
		}
	}

	for (const resource *discovered_resource : technology->get_enabled_resources()) {
		if (discovered_resource->is_prospectable()) {
			const point_set prospected_tiles = this->get_game_data()->get_prospected_tiles();
			for (const QPoint &tile_pos : prospected_tiles) {
				const tile *tile = map::get()->get_tile(tile_pos);

				if (tile->is_resource_discovered()) {
					continue;
				}

				if (!vector::contains(discovered_resource->get_terrain_types(), tile->get_terrain())) {
					continue;
				}

				this->get_game_data()->reset_tile_prospection(tile_pos);
			}
		} else {
			for (const province *province : this->get_game_data()->get_provinces()) {
				const province_game_data *province_game_data = province->get_game_data();

				if (!province_game_data->get_resource_counts().contains(discovered_resource)) {
					continue;
				}

				for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
					const tile *tile = map::get()->get_tile(tile_pos);
					const resource *tile_resource = tile->get_resource();

					if (tile_resource != discovered_resource) {
						continue;
					}

					if (!tile->is_resource_discovered()) {
						map::get()->set_tile_resource_discovered(tile_pos, true);
					}
				}
			}
		}
	}

	//check if any discoveries can now be triggered, if they required this technology
	bool leads_to_discovery = false;
	for (const metternich::technology *requiring_technology : technology->get_leads_to()) {
		if (requiring_technology->is_discovery()) {
			leads_to_discovery = true;
			break;
		}
	}

	if (leads_to_discovery) {
		for (const province *province : this->get_game_data()->get_explored_provinces()) {
			const province_game_data *province_game_data = province->get_game_data();

			for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
				const tile *tile = map::get()->get_tile(tile_pos);
				const resource *tile_resource = tile->get_resource();

				if (!tile->is_resource_discovered()) {
					continue;
				}

				if (tile_resource->get_discovery_technology() == nullptr) {
					continue;
				}

				if (this->can_gain_technology(tile_resource->get_discovery_technology())) {
					this->add_technology(tile_resource->get_discovery_technology());

					if (game::get()->is_running()) {
						emit technology_researched(tile_resource->get_discovery_technology());
					}
				}
			}
		}

		for (const QPoint &tile_pos : this->get_game_data()->get_explored_tiles()) {
			const tile *tile = map::get()->get_tile(tile_pos);
			const resource *tile_resource = tile->get_resource();

			if (tile_resource == nullptr) {
				continue;
			}

			if (!tile->is_resource_discovered()) {
				continue;
			}

			if (tile_resource->get_discovery_technology() == nullptr) {
				continue;
			}

			if (this->can_gain_technology(tile_resource->get_discovery_technology())) {
				this->add_technology(tile_resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit technology_researched(tile_resource->get_discovery_technology());
				}
			}
		}
	}

	if (game::get()->is_running()) {
		if (!technology->get_enabled_laws().empty()) {
			this->domain->get_government()->check_laws();
		}

		emit technologies_changed();
	}
}

void country_technology::remove_technology(const technology *technology)
{
	assert_throw(technology != nullptr);

	this->get_game_data()->get_capital_province()->get_game_data()->remove_technology(technology);
}

void country_technology::on_technology_lost(const technology *technology)
{
	if (technology->get_modifier() != nullptr) {
		technology->get_modifier()->apply(this->domain, -1);
	}

	for (const commodity *enabled_commodity : technology->get_enabled_commodities()) {
		if (!enabled_commodity->is_enabled()) {
			continue;
		}

		this->domain->get_economy()->remove_available_commodity(enabled_commodity);

		if (enabled_commodity->is_tradeable()) {
			this->domain->get_economy()->remove_tradeable_commodity(enabled_commodity);
		}
	}

	if (!technology->get_enabled_laws().empty()) {
		this->domain->get_government()->check_laws();
	}

	if (game::get()->is_running()) {
		emit technology_lost(technology);

		emit technologies_changed();
	}
}

void country_technology::check_technologies()
{
	//technologies may no longer be available for the country due to e.g. religion change, and may therefore need to be removed

	const technology_set technologies = this->get_technologies();
	for (const technology *technology : technologies) {
		if (!technology->is_available_for_country(this->domain)) {
			this->remove_technology(technology);
		}
	}
}

bool country_technology::can_gain_technology(const technology *technology) const
{
	assert_throw(technology != nullptr);

	if (!technology->is_available_for_country(this->domain)) {
		return false;
	}

	if (this->has_technology(technology)) {
		return false;
	}

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		if (!this->has_technology(prerequisite)) {
			return false;
		}
	}

	return true;
}

bool country_technology::can_research_technology(const technology *technology) const
{
	if (!this->can_gain_technology(technology)) {
		return false;
	}

	const int wealth_cost = technology->get_wealth_cost_for_country(this->domain);
	if (wealth_cost > 0 && wealth_cost > this->domain->get_economy()->get_wealth()) {
		return false;
	}

	for (const auto &[commodity, cost] : technology->get_commodity_costs_for_country(this->domain)) {
		if (cost > this->domain->get_economy()->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return true;
}

std::vector<const technology *> country_technology::get_researchable_technologies() const
{
	std::vector<const technology *> researchable_technologies;

	for (const technology *technology : this->domain->get_available_technologies()) {
		if (!this->is_technology_researchable(technology)) {
			continue;
		}

		researchable_technologies.push_back(technology);
	}

	std::sort(researchable_technologies.begin(), researchable_technologies.end(), technology_compare());

	return researchable_technologies;
}

QVariantList country_technology::get_researchable_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_researchable_technologies());
}

bool country_technology::is_technology_researchable(const technology *technology) const
{
	if (technology->is_discovery()) {
		return false;
	}

	return this->can_gain_technology(technology);
}

QVariantList country_technology::get_future_technologies_qvariant_list() const
{
	std::vector<const technology *> future_technologies = this->domain->get_available_technologies();
	std::erase_if(future_technologies, [this](const technology *technology) {
		if (this->has_technology(technology)) {
			return true;
		}

		bool has_all_prerequisites = true;
		for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
			if (!this->has_technology(prerequisite)) {
				has_all_prerequisites = false;
			}
		}
		if (has_all_prerequisites) {
			return true;
		}

		return false;
	});

	std::sort(future_technologies.begin(), future_technologies.end(), technology_compare());

	return container::to_qvariant_list(future_technologies);
}

QVariantList country_technology::get_current_researches_qvariant_list() const
{
	return container::to_qvariant_list(this->get_current_researches());
}

void country_technology::add_current_research(const technology *technology)
{
	assert_throw(this->can_research_technology(technology));

	const int wealth_cost = technology->get_wealth_cost_for_country(this->domain);
	this->domain->get_economy()->change_wealth(-wealth_cost);

	for (const auto &[commodity, cost] : technology->get_commodity_costs_for_country(this->domain)) {
		this->domain->get_economy()->change_stored_commodity(commodity, -cost);
	}

	this->current_researches.insert(technology);
	emit current_researches_changed();
}

void country_technology::remove_current_research(const technology *technology, const bool restore_costs)
{
	assert_throw(this->get_current_researches().contains(technology));

	if (restore_costs) {
		const int wealth_cost = technology->get_wealth_cost_for_country(this->domain);
		this->domain->get_economy()->change_wealth(wealth_cost);

		for (const auto &[commodity, cost] : technology->get_commodity_costs_for_country(this->domain)) {
			this->domain->get_economy()->change_stored_commodity(commodity, cost);
		}
	}

	this->current_researches.erase(technology);
	emit current_researches_changed();
}

void country_technology::on_technology_researched(const technology *technology)
{
	if (this->get_current_researches().contains(technology)) {
		this->remove_current_research(technology, false);
	}

	this->add_technology(technology);

	if (technology->get_free_technologies() > 0) {
		bool first_to_research = true;

		//technology grants a free technology for the first one to research it
		for (const metternich::domain *domain : game::get()->get_countries()) {
			if (domain == this->domain) {
				continue;
			}

			if (domain->get_technology()->has_technology(technology)) {
				first_to_research = false;
				break;
			}
		}

		if (first_to_research) {
			this->gain_free_technologies(technology->get_free_technologies());
		}
	}

	emit technology_researched(technology);
}

data_entry_map<technology_category, const technology *> country_technology::get_research_choice_map(const bool is_free) const
{
	const std::vector<const technology *> researchable_technologies = this->get_researchable_technologies();

	if (researchable_technologies.empty()) {
		return {};
	}

	data_entry_map<technology_category, std::vector<const technology *>> potential_technologies_per_category;

	for (const technology *technology : researchable_technologies) {
		if (!is_free && !this->can_research_technology(technology)) {
			continue;
		}

		std::vector<const metternich::technology *> &category_technologies = potential_technologies_per_category[technology->get_category()];

		const int weight = 1;
		for (int i = 0; i < weight; ++i) {
			category_technologies.push_back(technology);
		}
	}

	if (is_free) {
		assert_throw(!potential_technologies_per_category.empty());
	}

	if (potential_technologies_per_category.empty()) {
		return {};
	}

	data_entry_map<technology_category, const technology *> research_choice_map;
	const std::vector<const technology_category *> potential_categories = archimedes::map::get_keys(potential_technologies_per_category);

	for (const technology_category *category : potential_categories) {
		research_choice_map[category] = vector::get_random(potential_technologies_per_category[category]);
	}

	return research_choice_map;
}

void country_technology::gain_free_technology()
{
	const data_entry_map<technology_category, const technology *> research_choice_map = this->get_research_choice_map(true);

	if (research_choice_map.empty()) {
		return;
	}

	if (this->get_game_data()->is_ai()) {
		const technology *chosen_technology = this->domain->get_ai()->get_research_choice(research_choice_map);
		this->gain_free_technology(chosen_technology);
	} else {
		const std::vector<const technology *> potential_technologies = archimedes::map::get_values(research_choice_map);
		emit engine_interface::get()->free_technology_choosable(container::to_qvariant_list(potential_technologies));
	}
}

void country_technology::gain_free_technologies(const int count)
{
	assert_throw(count > 0);

	this->free_technology_count += count;
	this->gain_free_technology();
}

void country_technology::gain_technologies_known_by_others()
{
	static constexpr int min_countries = 2;

	technology_map<int> technology_known_counts;

	for (const metternich::domain *known_country : this->get_game_data()->get_known_countries()) {
		for (const technology *technology : known_country->get_technology()->get_technologies()) {
			++technology_known_counts[technology];
		}
	}

	for (const auto &[technology, known_count] : technology_known_counts) {
		if (known_count < min_countries) {
			continue;
		}

		if (this->has_technology(technology)) {
			continue;
		}

		this->on_technology_researched(technology);
	}
}

void country_technology::set_technology_category_cost_modifier(const technology_category *category, const centesimal_int &value)
{
	if (value == this->get_technology_category_cost_modifier(category)) {
		return;
	}

	if (value == 0) {
		this->technology_category_cost_modifiers.erase(category);
	} else {
		this->technology_category_cost_modifiers[category] = value;
	}
}

void country_technology::set_technology_subcategory_cost_modifier(const technology_subcategory *subcategory, const centesimal_int &value)
{
	if (value == this->get_technology_subcategory_cost_modifier(subcategory)) {
		return;
	}

	if (value == 0) {
		this->technology_subcategory_cost_modifiers.erase(subcategory);
	} else {
		this->technology_subcategory_cost_modifiers[subcategory] = value;
	}
}

void country_technology::set_gain_technologies_known_by_others_count(const int value)
{
	const int old_value = this->get_gain_technologies_known_by_others_count();
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	this->gain_technologies_known_by_others_count = value;

	if (old_value == 0) {
		this->gain_technologies_known_by_others();
	}
}

}
