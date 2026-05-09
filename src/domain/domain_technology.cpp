#include "metternich.h"

#include "domain/domain_technology.h"

#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_ai.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
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
#include "population/population.h"
#include "population/population_type.h"
#include "script/context.h"
#include "script/effect/effect_list.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "ui/portrait.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/point_container.h"
#include "util/map_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace metternich {

domain_technology::domain_technology(const metternich::domain *domain, const domain_game_data *game_data)
	: domain(domain)
{
	connect(game_data, &domain_game_data::available_idea_slots_changed, this, &domain_technology::available_research_slots_changed);
}

domain_technology::~domain_technology()
{
}

void domain_technology::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "free_technology_count") {
		this->free_technology_count = std::stoi(value);
	} else {
		throw std::runtime_error(std::format("Invalid domain technology property: \"{}\".", key));
	}
}

void domain_technology::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "current_researches") {
		for (const std::string &value : values) {
			this->current_researches.insert(technology::get(value));
		}
	} else if (tag == "current_research_progresses") {
		scope.for_each_property([this](const gsml_property &property) {
			this->current_research_progresses[technology::get(property.get_key())] = decimillesimal_int(property.get_value());
		});
	} else {
		throw std::runtime_error(std::format("Invalid domain technology scope: \"{}\".", tag));
	}
}

gsml_data domain_technology::to_gsml_data() const
{
	gsml_data data("technology");

	if (this->free_technology_count != 0) {
		data.add_property("free_technology_count", std::to_string(this->free_technology_count));
	}

	if (!this->get_current_researches().empty()) {
		gsml_data current_researches_data("current_researches");
		for (const technology *technology : this->get_current_researches()) {
			current_researches_data.add_value(technology->get_identifier());
		}
		data.add_child(std::move(current_researches_data));
	}

	if (!this->current_research_progresses.empty()) {
		gsml_data current_research_progresses_data("current_research_progresses");
		for (const auto &[technology, progress] : this->current_research_progresses) {
			current_research_progresses_data.add_property(technology->get_identifier(), progress.to_string());
		}
		data.add_child(std::move(current_research_progresses_data));
	}

	return data;
}

domain_game_data *domain_technology::get_game_data() const
{
	return this->domain->get_game_data();
}

QCoro::Task<void> domain_technology::do_research()
{
	try {
		if (this->get_gain_technologies_known_by_others_count() > 0) {
			co_await this->gain_technologies_known_by_others();
		}

		assert_throw(this->free_technology_count >= 0);

		if (this->get_current_researches().empty() && this->free_technology_count == 0) {
			this->choose_current_research();
			co_return;
		}

		if (this->free_technology_count > 0) {
			co_await this->gain_free_technology();
		}

		if (this->get_current_researches().empty()) {
			co_return;
		}

		decimillesimal_int generated_research = decimillesimal_int(this->get_game_data()->get_economy()->get_stored_commodity(defines::get()->get_default_research_commodity()));
		this->get_game_data()->get_economy()->set_stored_commodity(defines::get()->get_default_research_commodity(), 0);

		while (generated_research > 0 && !this->get_current_researches().empty()) {
			decimillesimal_int remaining_generated_research = generated_research;

			const technology_set current_researches = this->get_current_researches();
			for (const technology *current_research : current_researches) {
				const commodity_map<int64_t> commodity_costs = current_research->get_commodity_costs_for_domain(this->domain);
				if (!commodity_costs.contains(defines::get()->get_default_research_commodity())) {
					continue;
				}

				const decimillesimal_int research_cost = decimillesimal_int(commodity_costs.find(defines::get()->get_default_research_commodity())->second);
				const decimillesimal_int remaining_research = research_cost * (100 - this->get_current_research_progress(current_research) / 100);
				const decimillesimal_int research_for_progress = decimillesimal_int::min(remaining_research, generated_research / current_researches.size());
				this->change_current_research_progress(current_research, research_for_progress * 100 / research_cost);
				remaining_generated_research -= research_for_progress;

				if (this->get_current_research_progress(current_research) >= 100) {
					co_await this->on_technology_researched(current_research);
				}
			}

			generated_research = remaining_generated_research;
			if (!this->get_current_researches().empty() && (generated_research / this->get_current_researches().size()) == 0) {
				break;
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing research for domain \"" + this->domain->get_identifier() + "\"."));
	}
}

QCoro::Task<void> domain_technology::do_technology_spread()
{
	province_map<std::vector<const technology *>> province_spread_technologies;

	for (const province *province : this->get_game_data()->get_provinces()) {
		technology_map<decimillesimal_int> technology_spread_bonuses;

		for (const metternich::province *neighbor_province : province->get_game_data()->get_neighbor_provinces()) {
			for (const technology *technology : neighbor_province->get_game_data()->get_technologies()) {
				if (!province->get_game_data()->can_gain_technology(technology)) {
					continue;
				}

				technology_spread_bonuses[technology] += 6 * decimillesimal_int(neighbor_province->get_game_data()->get_extra_technology(technology) + 1);
			}
		}

		for (auto &[technology, spread_bonus] : technology_spread_bonuses) {
			const int spread_modifier = province->get_game_data()->get_technology_category_spread_modifier(technology->get_category());
			if (spread_modifier != 0) {
				spread_bonus *= 100 + spread_modifier;
				spread_bonus /= 100;
			}
		}

		for (const auto &[technology, spread_bonus] : technology_spread_bonuses) {
			static const decimillesimal_int base_spread_years(10);
			static const decimillesimal_int base_spread_months = base_spread_years * 12;
			decimillesimal_int mtth = base_spread_months / game::get()->get_current_months_per_turn();
			mtth *= 100;
			mtth /= spread_bonus;

			if (technology->get_spread_mean_time_to_happen_factor() != nullptr) {
				mtth = technology->get_spread_mean_time_to_happen_factor()->calculate(province, mtth);
			}

			bool should_spread = false;

			if (mtth <= 1) {
				should_spread = true;
			} else {
				const int64_t spread_chance = (decimillesimal_int(1) / mtth).get_value();
				assert_throw(spread_chance > 0);
				should_spread = random::get()->generate(10000ll) < spread_chance;
			}

			if (should_spread) {
				co_await province->get_game_data()->add_technology(technology);
				province_spread_technologies[province].push_back(technology);
			}
		}
	}

	if (this->domain == game::get()->get_player_country() && !province_spread_technologies.empty()) {
		const portrait *interior_minister_portrait = this->get_game_data()->get_government()->get_interior_minister_portrait();

		std::string spread_technologies_str;
		for (const auto &[province, spread_technologies] : province_spread_technologies) {
			for (const technology *technology : spread_technologies) {
				spread_technologies_str += std::format("\n{} to {}", technology->get_name(), province->get_game_data()->get_current_cultural_name());
			}
		}

		engine_interface::get()->add_notification("Technology Spread", interior_minister_portrait, std::format("{}, new technologies have spread to our provinces!\n\nSpread Technologies:{}", this->get_game_data()->get_form_of_address(), spread_technologies_str));
	}
}

void domain_technology::do_population_research()
{
	//generate research points based on population

	const population *domain_population = this->get_game_data()->get_population();

	if (domain_population->get_size() == 0) {
		return;
	}
	
	const commodity *research_commodity = defines::get()->get_default_research_commodity();
	assert_throw(research_commodity != nullptr);

	decimillesimal_int daily_research = decimillesimal_int(defines::get()->get_daily_literacy_research()) * domain_population->get_literacy_rate() / 100;

	for (const auto &[population_type, size] : domain_population->get_type_sizes()) {
		if (population_type->get_daily_research() != 0) {
			const decimillesimal_int population_type_percent = decimillesimal_int(size) * 100 / domain_population->get_size();
			daily_research += population_type->get_daily_research() * decimillesimal_int::min(population_type_percent, population_type->get_max_research_population_percent()) / population_type->get_max_research_population_percent();
		}
	}

	const int turn_days = game::get()->get_days_until_next_turn();
	const int64_t generated_research = (daily_research * turn_days).to_int64();
	this->get_game_data()->get_economy()->change_stored_commodity(research_commodity, generated_research);
}

const technology_set &domain_technology::get_technologies() const
{
	if (this->get_game_data()->get_capital_province() != nullptr) {
		return this->get_game_data()->get_capital_province()->get_game_data()->get_technologies();
	} else if (this->domain->get_default_capital()->get_province()->get_game_data()->is_on_map()) {
		return this->domain->get_default_capital()->get_province()->get_game_data()->get_technologies();
	}

	static const technology_set empty_set;
	return empty_set;
}

QVariantList domain_technology::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

QCoro::Task<void> domain_technology::add_technology(const technology *technology)
{
	if (this->get_game_data()->get_capital_province() != nullptr) {
		co_await this->get_game_data()->get_capital_province()->get_game_data()->add_technology(technology);
	} else if (this->domain->get_default_capital()->get_province()->get_game_data()->is_on_map()) {
		co_await this->domain->get_default_capital()->get_province()->get_game_data()->add_technology(technology);
	}
}

QCoro::Task<void> domain_technology::add_technology_with_prerequisites(const technology *technology)
{
	co_await this->add_technology(technology);

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		co_await this->add_technology_with_prerequisites(prerequisite);
	}
}

QCoro::Task<void> domain_technology::on_technology_added(const technology *technology)
{
	if (technology->get_domain_modifier() != nullptr) {
		co_await technology->get_domain_modifier()->apply(this->domain, 1);
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
						co_await map::get()->set_tile_resource_discovered(tile_pos, true);
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
					co_await this->add_technology(tile_resource->get_discovery_technology());

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
				co_await this->add_technology(tile_resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit technology_researched(tile_resource->get_discovery_technology());
				}
			}
		}
	}

	if (game::get()->is_running()) {
		if (!technology->get_enabled_laws().empty()) {
			co_await this->domain->get_government()->check_laws();
		}

		emit technologies_changed();
	}
}

QCoro::Task<void> domain_technology::remove_technology(const technology *technology)
{
	assert_throw(technology != nullptr);

	co_await this->get_game_data()->get_capital_province()->get_game_data()->remove_technology(technology);
}

QCoro::Task<void> domain_technology::on_technology_lost(const technology *technology)
{
	if (technology->get_domain_modifier() != nullptr) {
		co_await technology->get_domain_modifier()->apply(this->domain, -1);
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
		co_await this->domain->get_government()->check_laws();
	}

	if (game::get()->is_running()) {
		emit technology_lost(technology);

		emit technologies_changed();
	}
}

QCoro::Task<void> domain_technology::check_technologies()
{
	//technologies may no longer be available for the country due to e.g. religion change, and may therefore need to be removed

	const technology_set technologies = this->get_technologies();
	for (const technology *technology : technologies) {
		if (!technology->is_available_for_domain(this->domain)) {
			co_await this->remove_technology(technology);
		}
	}
}

bool domain_technology::can_gain_technology(const technology *technology) const
{
	assert_throw(technology != nullptr);

	if (!technology->is_available_for_domain(this->domain)) {
		return false;
	}

	if (this->get_game_data()->get_capital_province() == nullptr) {
		return false;
	}

	return this->get_game_data()->get_capital_province()->get_game_data()->can_gain_technology(technology);
}

bool domain_technology::can_research_technology(const technology *technology) const
{
	if (!this->can_gain_technology(technology)) {
		return false;
	}

	for (const auto &[commodity, cost] : technology->get_commodity_costs_for_domain(this->domain)) {
		if (commodity == defines::get()->get_default_research_commodity()) {
			continue;
		}

		if (cost > this->domain->get_economy()->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return true;
}

std::vector<const technology *> domain_technology::get_researchable_technologies() const
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

QVariantList domain_technology::get_researchable_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_researchable_technologies());
}

bool domain_technology::is_technology_researchable(const technology *technology) const
{
	if (technology->is_discovery()) {
		return false;
	}

	if (technology->get_level() == 0) {
		return false;
	}

	return this->can_gain_technology(technology);
}

QVariantList domain_technology::get_future_technologies_qvariant_list() const
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
		if (has_all_prerequisites && this->is_technology_researchable(technology)) {
			return true;
		}

		return false;
	});

	std::sort(future_technologies.begin(), future_technologies.end(), technology_compare());

	return container::to_qvariant_list(future_technologies);
}

QVariantList domain_technology::get_current_researches_qvariant_list() const
{
	return container::to_qvariant_list(this->get_current_researches());
}

void domain_technology::add_current_research(const technology *technology)
{
	assert_throw(this->can_research_technology(technology));

	for (const auto &[commodity, cost] : technology->get_commodity_costs_for_domain(this->domain)) {
		if (commodity == defines::get()->get_default_research_commodity()) {
			continue;
		}

		this->domain->get_economy()->change_stored_commodity(commodity, -cost);
	}

	this->current_researches.insert(technology);
	if (!this->current_research_progresses.contains(technology)) {
		this->current_research_progresses[technology] = decimillesimal_int(0);
	}
	emit current_researches_changed();
}

void domain_technology::remove_current_research(const technology *technology, const bool restore_costs, const bool preserve_progress)
{
	assert_throw(this->get_current_researches().contains(technology));

	if (restore_costs) {
		for (const auto &[commodity, cost] : technology->get_commodity_costs_for_domain(this->domain)) {
			if (commodity == defines::get()->get_default_research_commodity()) {
				continue;
			}

			this->domain->get_economy()->change_stored_commodity(commodity, cost);
		}
	}

	this->current_researches.erase(technology);
	if (!preserve_progress) {
		this->current_research_progresses.erase(technology);
	}
	emit current_researches_changed();
}

const decimillesimal_int &domain_technology::get_current_research_progress(const technology *technology) const
{
	const auto find_iterator = this->current_research_progresses.find(technology);

	assert_throw(find_iterator != this->current_research_progresses.end());

	return find_iterator->second;
}

qint64 domain_technology::get_current_research_progress_commodity_quantity(const technology *technology) const
{
	const commodity_map<int64_t> commodity_costs = technology->get_commodity_costs_for_domain(this->domain);
	if (!commodity_costs.contains(defines::get()->get_default_research_commodity())) {
		return 0;
	}

	return (commodity_costs.find(defines::get()->get_default_research_commodity())->second * this->get_current_research_progress(technology) / 100).to_int64();
}

QString domain_technology::get_current_research_progress_qstring(const technology *technology) const
{
	return QString::fromStdString(std::to_string(this->get_current_research_progress(technology).to_int()));
}

void domain_technology::change_current_research_progress(const technology *technology, const decimillesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const decimillesimal_int &new_value = (this->current_research_progresses[technology] += change);
	assert_throw(new_value >= 0);
	if (new_value == 0) {
		this->current_research_progresses.erase(technology);
	}
}

void domain_technology::choose_current_research()
{
	const data_entry_map<technology_category, const technology *> research_choice_map = this->domain->get_technology()->get_research_choice_map(false);

	if (research_choice_map.empty()) {
		return;
	}

	if (this->get_game_data()->is_ai()) {
		const technology *chosen_technology = this->get_game_data()->get_ai()->get_research_choice(research_choice_map);

		if (chosen_technology != nullptr) {
			this->domain->get_technology()->add_current_research(chosen_technology);
		}
	} else {
		const std::vector<const technology *> potential_technologies = archimedes::map::get_values(research_choice_map);
		emit engine_interface::get()->technology_choosable(container::to_qvariant_list(potential_technologies));
	}
}

QCoro::Task<void> domain_technology::on_technology_researched(const technology *technology)
{
	if (this->get_current_researches().contains(technology)) {
		this->remove_current_research(technology, false, false);
	}

	co_await this->add_technology(technology);

	if (technology->get_discovery_effects() != nullptr) {
		context ctx(this->domain);
		co_await technology->get_discovery_effects()->do_effects(this->domain, ctx);
	}

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
			co_await this->gain_free_technologies(technology->get_free_technologies());
		}
	}

	emit technology_researched(technology);
}

data_entry_map<technology_category, const technology *> domain_technology::get_research_choice_map(const bool is_free) const
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

		const int weight = technology::get_max_level() + 1 - technology->get_level();
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

QCoro::Task<void> domain_technology::gain_free_technology()
{
	const data_entry_map<technology_category, const technology *> research_choice_map = this->get_research_choice_map(true);

	if (research_choice_map.empty()) {
		co_return;
	}

	if (this->get_game_data()->is_ai()) {
		const technology *chosen_technology = this->domain->get_ai()->get_research_choice(research_choice_map);
		co_await this->gain_free_technology_coro(chosen_technology);
	} else {
		const std::vector<const technology *> potential_technologies = archimedes::map::get_values(research_choice_map);
		emit engine_interface::get()->free_technology_choosable(container::to_qvariant_list(potential_technologies));
	}
}

QCoro::Task<void> domain_technology::gain_free_technologies(const int count)
{
	assert_throw(count > 0);

	this->free_technology_count += count;
	co_await this->gain_free_technology();
}

QCoro::Task<void> domain_technology::gain_technologies_known_by_others()
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

		co_await this->on_technology_researched(technology);
	}
}

void domain_technology::set_technology_category_cost_modifier(const technology_category *category, const centesimal_int &value)
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

void domain_technology::set_technology_subcategory_cost_modifier(const technology_subcategory *subcategory, const centesimal_int &value)
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

QCoro::Task<void> domain_technology::set_gain_technologies_known_by_others_count(const int value)
{
	const int old_value = this->get_gain_technologies_known_by_others_count();
	if (value == old_value) {
		co_return;
	}

	assert_throw(value >= 0);

	this->gain_technologies_known_by_others_count = value;

	if (old_value == 0) {
		co_await this->gain_technologies_known_by_others();
	}
}

}
