#include "metternich.h"

#include "technology/technology.h"

#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_technology.h"
#include "domain/government_type.h"
#include "domain/law.h"
#include "economy/commodity.h"
#include "economy/employment_type.h"
#include "economy/resource.h"
#include "game/event_option.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "game/province_event.h"
#include "infrastructure/building_type.h"
#include "infrastructure/pathway.h"
#include "infrastructure/wonder.h"
#include "item/item_type.h"
#include "map/terrain_type.h"
#include "religion/deity.h"
#include "religion/religion.h"
#include "religion/religious_group.h"
#include "script/condition/and_condition.h"
#include "script/condition/can_gain_technology_condition.h"
#include "script/condition/capital_condition.h"
#include "script/condition/event_condition.h"
#include "script/condition/source_province_scope_condition.h"
#include "script/condition/technology_condition.h"
#include "script/effect/effect_list.h"
#include "script/effect/technologies_effect.h"
#include "script/factor.h"
#include "script/mean_time_to_happen.h"
#include "script/modifier.h"
#include "spell/spell.h"
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

	const auto sort_function = [](const technology *lhs, const technology *rhs) {
		if (lhs->get_category() != rhs->get_category() && lhs->get_category() != nullptr && rhs->get_category() != nullptr) {
			return lhs->get_category()->get_identifier() < rhs->get_category()->get_identifier();
		}

		if (lhs->get_subcategory() != rhs->get_subcategory() && lhs->get_subcategory() != nullptr && rhs->get_subcategory() != nullptr) {
			return lhs->get_subcategory()->get_identifier() < rhs->get_subcategory()->get_identifier();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	};

	const auto named_data_entry_sort_function = [&sort_function](const named_data_entry *lhs, const named_data_entry *rhs) {
		return sort_function(static_cast<const technology *>(lhs), static_cast<const technology *>(rhs));
	};

	technology::sort_instances(sort_function);
	std::sort(technology::top_level_technologies.begin(), technology::top_level_technologies.end(), sort_function);

	for (technology *technology : technology::get_all()) {
		if (!technology->child_technologies.empty()) {
			std::sort(technology->child_technologies.begin(), technology->child_technologies.end(), sort_function);
			std::sort(technology->get_tree_children().begin(), technology->get_tree_children().end(), named_data_entry_sort_function);
		}
	}

	commodity_map<int> research_commodity_counts;
	for (const technology *technology : technology::get_all()) {
		for (const auto &[commodity, cost] : technology->commodity_costs) {
			++research_commodity_counts[commodity];
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
	} else if (tag == "religions") {
		for (const std::string &value : values) {
			this->religions.insert(religion::get(value));
		}
	} else if (tag == "religious_groups") {
		for (const std::string &value : values) {
			this->religious_groups.push_back(religious_group::get(value));
		}
	} else if (tag == "prerequisites") {
		for (const std::string &value : values) {
			this->prerequisites.push_back(technology::get(value));
		}
	} else if (tag == "cost_factor") {
		auto factor = std::make_unique<metternich::factor<province>>(100);
		factor->process_gsml_data(scope);
		this->cost_factor = std::move(factor);
	} else if (tag == "spread_mean_time_to_happen_factor") {
		auto factor = std::make_unique<metternich::factor<province>>(100);
		factor->process_gsml_data(scope);
		this->spread_mean_time_to_happen_factor = std::move(factor);
	} else if (tag == "weight_factor") {
		auto factor = std::make_unique<metternich::factor<domain>>(100);
		factor->process_gsml_data(scope);
		this->weight_factor = std::move(factor);
	} else if (tag == "shared_commodities") {
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->shared_commodities[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const province>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else if (tag == "domain_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const domain>>();
		modifier->process_gsml_data(scope);
		this->domain_modifier = std::move(modifier);
	} else if (tag == "discovery_conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		conditions->process_gsml_data(scope);
		this->discovery_conditions = std::move(conditions);
	} else if (tag == "discovery_effects") {
		auto effects = std::make_unique<effect_list<const province>>();
		effects->process_gsml_data(scope);
		this->discovery_effects = std::move(effects);
	} else if (tag == "spread_conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		conditions->process_gsml_data(scope);
		this->spread_conditions = std::move(conditions);
	} else if (tag == "discovery_mean_time_to_happen") {
		this->discovery_mean_time_to_happen = std::make_unique<metternich::mean_time_to_happen<province>>();
		scope.process(this->discovery_mean_time_to_happen.get());
	} else if (tag == "discovery_monthly_chance") {
		this->discovery_monthly_chance = std::make_unique<metternich::factor<province>>();
		scope.process(this->discovery_monthly_chance.get());
	} else if (tag == "discovery_yearly_chance") {
		this->discovery_yearly_chance = std::make_unique<metternich::factor<province>>();
		scope.process(this->discovery_yearly_chance.get());
	} else if (tag == "spread_mean_time_to_happen") {
		this->spread_mean_time_to_happen = std::make_unique<metternich::mean_time_to_happen<province>>();
		scope.process(this->spread_mean_time_to_happen.get());
	} else if (tag == "spread_monthly_chance") {
		this->spread_monthly_chance = std::make_unique<metternich::factor<province>>();
		scope.process(this->spread_monthly_chance.get());
	} else if (tag == "spread_yearly_chance") {
		this->spread_yearly_chance = std::make_unique<metternich::factor<province>>();
		scope.process(this->spread_yearly_chance.get());
	} else if (tag == "monthly_chance") {
		this->discovery_monthly_chance = std::make_unique<metternich::factor<province>>();
		scope.process(this->discovery_monthly_chance.get());

		this->spread_monthly_chance = std::make_unique<metternich::factor<province>>();
		scope.process(this->spread_monthly_chance.get());
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void technology::initialize()
{
	if (this->subcategory != nullptr) {
		this->subcategory->add_technology(this);
	}
	
	if (!this->get_prerequisites().empty()) {
		technology *parent_technology = this->get_prerequisites().at(0);
		this->parent_technology = parent_technology;
		parent_technology->child_technologies.push_back(this);
	} else {
		technology::top_level_technologies.push_back(this);
	}

	this->calculate_total_prerequisite_depth();

	for (technology *prerequisite : this->get_prerequisites()) {
		prerequisite->leads_to.push_back(this);
	}

	std::sort(this->enabled_pathways.begin(), this->enabled_pathways.end(), pathway_compare());
	std::sort(this->enabled_river_crossing_pathways.begin(), this->enabled_river_crossing_pathways.end(), pathway_compare());

	//if (this->get_level() == 0) {
	//	this->level = this->get_total_prerequisite_depth() + 1;
	//}

	technology::max_level = std::max(technology::max_level, this->get_level());

	if (this->get_level() > 0) {
		this->commodity_costs[defines::get()->get_default_research_commodity()] = defines::get()->get_research_cost_per_level() * this->get_level();
	}

	if (this->discovery_mean_time_to_happen != nullptr || this->discovery_monthly_chance != nullptr || this->discovery_yearly_chance != nullptr) {
		province_event *event = province_event::add(std::format("{}_discovered", this->get_identifier()), this->get_module());
		event->set_name(std::format("{} Discovered", this->get_name()));
		event->set_portrait(this->get_portrait());
		event->set_description(std::format("[root.domain.form_of_address], the {} technology has been discovered in [root.name]!", string::lowered(this->get_name())));
		event->set_only_once(this->is_discovered_only_once());
		if (this->discovery_monthly_chance != nullptr) {
			event->set_monthly_chance(std::move(this->discovery_monthly_chance));
		} else if (this->discovery_yearly_chance != nullptr) {
			event->set_yearly_chance(std::move(this->discovery_yearly_chance));
		} else {
			event->set_mean_time_to_happen(std::move(this->discovery_mean_time_to_happen));
		}

		auto event_conditions = std::make_unique<and_condition<province>>();
		event_conditions->add_condition(std::make_unique<capital_condition<province>>(true));
		event_conditions->add_condition(std::make_unique<can_gain_technology_condition<province>>(this));
		if (this->discovery_conditions != nullptr) {
			event_conditions->add_condition(std::move(this->discovery_conditions));
			this->discovery_conditions = nullptr;
		}
		event->set_conditions(std::move(event_conditions));

		auto event_option = std::make_unique<metternich::event_option<const province>>();
		event_option->add_effect(std::make_unique<technologies_effect<const province>>(this, gsml_operator::addition));
		if (this->discovery_effects != nullptr) {
			event_option->add_effects(std::move(this->discovery_effects));
			this->discovery_effects = nullptr;
		}
		event->add_option(std::move(event_option));

		event->initialize();

		this->discovery_event = event;
	}

	if (this->spread_mean_time_to_happen != nullptr || this->spread_monthly_chance != nullptr || this->spread_yearly_chance != nullptr) {
		assert_throw(this->get_discovery_event() != nullptr);

		province_event *event = province_event::add(std::format("{}_spread", this->get_identifier()), this->get_module());
		event->set_name(std::format("{} Spread to [root.name]", this->get_name()));
		event->set_portrait(this->get_portrait());
		event->set_description(std::format("[root.domain.form_of_address], the {} technology has spread to [root.name].", string::lowered(this->get_name())));
		event->set_from_neighbor(this->spread_from_neighbor);
		event->set_spread_technology(this);
		event->set_hidden(true); //too many events could be displayed for a large domain if this is not hidden
		if (this->spread_monthly_chance != nullptr) {
			event->set_monthly_chance(std::move(this->spread_monthly_chance));
		} else if (this->spread_yearly_chance != nullptr) {
			event->set_yearly_chance(std::move(this->spread_yearly_chance));
		} else {
			event->set_mean_time_to_happen(std::move(this->spread_mean_time_to_happen));
		}

		auto event_conditions = std::make_unique<and_condition<province>>();
		event_conditions->add_condition(std::make_unique<event_condition<province>>(this->get_discovery_event())); //the discovery event must have happened
		event_conditions->add_condition(std::make_unique<can_gain_technology_condition<province>>(this));
		if (this->spread_from_neighbor) {
			auto source_condition = std::make_unique<source_province_scope_condition<province>>();
			source_condition->add_condition(std::make_unique<technology_condition<province>>(this));
			event_conditions->add_condition(std::move(source_condition));
		}
		if (this->spread_conditions != nullptr) {
			event_conditions->add_condition(std::move(this->spread_conditions));
		}
		event->set_conditions(std::move(event_conditions));

		auto event_option = std::make_unique<metternich::event_option<const province>>();
		event_option->add_effect(std::make_unique<technologies_effect<const province>>(this, gsml_operator::addition));
		event->add_option(std::move(event_option));

		event->initialize();

		this->spread_event = event;
	}

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

	if (this->get_level() == 0 && this->get_discovery_event() == nullptr) {
		log::log_error(std::format("Technology \"{}\" has neither a level for research, nor a discovery event.", this->get_identifier()));
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
	} else {
		log::log_error(std::format("Technology \"{}\" has no period.", this->get_identifier()));
	}

	if (
		this->get_modifier() == nullptr
		&& this->get_domain_modifier() == nullptr
		&& this->get_free_technologies() == 0
		&& this->get_enabled_buildings().empty()
		&& this->get_enabled_civilian_units().empty()
		&& this->get_enabled_commodities().empty()
		&& this->get_enabled_deities().empty()
		&& this->get_enabled_employment_types().empty()
		&& this->get_enabled_government_types().empty()
		&& this->get_enabled_item_types().empty()
		&& this->get_enabled_laws().empty()
		&& this->get_enabled_military_units().empty()
		&& this->get_enabled_pathway_terrains().empty()
		&& this->get_enabled_pathways().empty()
		&& this->get_enabled_resources().empty()
		&& this->get_enabled_river_crossing_pathways().empty()
		&& this->get_enabled_spells().empty()
		&& this->get_enabled_transporters().empty()
		&& this->get_enabled_wonders().empty()
		&& this->get_shared_commodities().empty()
	) {
		if (this->get_leads_to().empty()) {
			log::log_error(std::format("Technology \"{}\" has no effects, and does not lead to any other technologies.", this->get_identifier()));
		}
	}
}

const technology_category *technology::get_category() const
{
	if (this->get_subcategory() != nullptr) {
		return this->get_subcategory()->get_category();
	}

	return nullptr;
}

bool technology::is_available_for_domain(const domain *domain) const
{
	if (!this->is_enabled()) {
		return false;
	}

	if (!this->cultures.empty() || !this->cultural_groups.empty()) {
		if (this->cultures.contains(domain->get_game_data()->get_culture())) {
			return true;
		}

		for (const cultural_group *cultural_group : this->cultural_groups) {
			if (domain->get_game_data()->get_culture()->is_part_of_group(cultural_group)) {
				return true;
			}
		}

		return false;
	}

	if (!this->religions.empty() || !this->religious_groups.empty()) {
		if (this->religions.contains(domain->get_game_data()->get_religion())) {
			return true;
		}

		if (vector::contains(this->religious_groups, domain->get_game_data()->get_religion()->get_group())) {
			return true;
		}

		return false;
	}

	return true;
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

commodity_map<int64_t> technology::get_commodity_costs_for_domain(const domain *domain) const
{
	assert_throw(domain->get_game_data()->get_capital_province() != nullptr);

	commodity_map<int64_t> costs;

	for (const auto &[commodity, base_cost] : this->get_commodity_costs()) {
		if (!commodity->is_enabled()) {
			continue;
		}

		if (base_cost <= 0) {
			continue;
		}

		decimillesimal_int cost(base_cost);
		cost *= decimillesimal_int(centesimal_int(100) + domain->get_technology()->get_technology_cost_modifier() + domain->get_technology()->get_technology_category_cost_modifier(this->get_category()) + domain->get_technology()->get_technology_subcategory_cost_modifier(this->get_subcategory()));
		cost /= 100;

		if (this->get_cost_factor() != nullptr) {
			cost = this->get_cost_factor()->calculate(domain->get_game_data()->get_capital_province(), cost);
		}

		costs[commodity] = std::max(1ll, cost.to_int64());
	}

	return costs;
}

QVariantList technology::get_commodity_costs_for_domain_qvariant_list(const domain *domain) const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_costs_for_domain(domain));
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

std::vector<const wonder *> technology::get_enabled_wonders_for_country(const domain *domain) const
{
	std::vector<const wonder *> wonders;

	for (const wonder *wonder : this->get_enabled_wonders()) {
		if (wonder->get_conditions() != nullptr && !wonder->get_conditions()->check(domain, read_only_context(domain))) {
			continue;
		}

		wonders.push_back(wonder);
	}

	return wonders;
}

std::vector<const wonder *> technology::get_disabled_wonders_for_country(const domain *domain) const
{
	std::vector<const wonder *> wonders;

	for (const wonder *wonder : this->get_disabled_wonders()) {
		if (wonder->get_conditions() != nullptr && !wonder->get_conditions()->check(domain, read_only_context(domain))) {
			continue;
		}

		wonders.push_back(wonder);
	}

	return wonders;
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

void technology::add_enabled_government_type(const government_type *government_type)
{
	this->enabled_government_types.push_back(government_type);

	std::sort(this->enabled_government_types.begin(), this->enabled_government_types.end(), [](const metternich::government_type *lhs, const metternich::government_type *rhs) {
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

std::vector<const deity *> technology::get_enabled_deities_for_country(const domain *domain) const
{
	std::vector<const deity *> deities;

	for (const deity *deity : this->get_enabled_deities()) {
		if (deity->get_conditions() != nullptr && !deity->get_conditions()->check(domain, read_only_context(domain))) {
			continue;
		}

		deities.push_back(deity);
	}

	return deities;
}

std::vector<const deity *> technology::get_disabled_deities_for_country(const domain *domain) const
{
	std::vector<const deity *> deities;

	for (const deity *deity : this->get_disabled_deities()) {
		if (deity->get_conditions() != nullptr && !deity->get_conditions()->check(domain, read_only_context(domain))) {
			continue;
		}

		deities.push_back(deity);
	}

	return deities;
}

commodity_map<int64_t> technology::get_shared_commodities_for_domain(const domain *domain) const
{
	commodity_map<int64_t> commodities = this->get_shared_commodities();

	if (commodities.empty()) {
		return commodities;
	}

	for (const metternich::domain *loop_country : game::get()->get_countries()) {
		if (loop_country == domain) {
			continue;
		}

		if (loop_country->get_technology()->has_technology(this)) {
			bool all_commodities_at_lowest = true;
			for (auto &[commodity, value] : commodities) {
				value /= 2;
				value = std::max(value, 1ll);
				all_commodities_at_lowest = (all_commodities_at_lowest && value == 1);
			}

			if (all_commodities_at_lowest) {
				break;
			}
		}
	}

	return commodities;
}

std::string technology::get_modifier_string(const province *province) const
{
	assert_throw(province != nullptr);

	std::string str;

	if (this->get_modifier() != nullptr) {
		str = this->get_modifier()->get_string(province);
	}

	if (this->get_domain_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str = this->get_domain_modifier()->get_string(province->get_game_data()->get_owner());
	}

	return str;
}

std::string technology::get_effects_string(const domain *domain) const
{
	const province *capital_province = domain->get_game_data()->get_capital_province();
	assert_throw(capital_province != nullptr);
	std::string str = this->get_modifier_string(capital_province);

	if (this->get_discovery_effects() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_discovery_effects()->get_effects_string(capital_province, read_only_context(capital_province));
	}

	if (this->get_free_technologies() > 0) {
		if (!str.empty()) {
			str += "\n";
		}

		str += std::format("{} free {} for the first to research", this->get_free_technologies(), this->get_free_technologies() > 1 ? "technologies" : "technology");
	}

	if (!this->get_shared_commodities().empty()) {
		for (const auto &[commodity, value] : this->get_shared_commodities_for_domain(domain)) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("+{} {}", value, commodity->get_name());
		}
	}

	if (!this->get_enabled_commodities().empty()) {
		for (const commodity *commodity : this->get_enabled_commodities()) {
			if (!commodity->is_enabled()) {
				continue;
			}

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

	const std::vector<const building_type *> buildings = this->get_enabled_buildings_for_culture(domain->get_game_data()->get_culture());
	if (!buildings.empty()) {
		for (const building_type *building : buildings) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} building", building->get_name());
		}
	}

	const std::vector<const wonder *> enabled_wonders = this->get_enabled_wonders_for_country(domain);
	if (!enabled_wonders.empty()) {
		for (const wonder *wonder : enabled_wonders) {
			if (!wonder->is_enabled()) {
				continue;
			}

			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} wonder", wonder->get_name());
		}
	}

	const std::vector<const wonder *> disabled_wonders = this->get_disabled_wonders_for_country(domain);
	if (!disabled_wonders.empty()) {
		for (const wonder *wonder : disabled_wonders) {
			if (!wonder->is_enabled()) {
				continue;
			}

			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Obsoletes {} wonder", wonder->get_name());
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

	const std::vector<const civilian_unit_type *> civilian_units = this->get_enabled_civilian_units_for_culture(domain->get_game_data()->get_culture());
	if (!civilian_units.empty()) {
		for (const civilian_unit_type *civilian_unit : civilian_units) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} civilian unit", civilian_unit->get_name());
		}
	}

	const std::vector<const military_unit_type *> military_units = get_enabled_military_units_for_culture(domain->get_game_data()->get_culture());
	if (!military_units.empty()) {
		for (const military_unit_type *military_unit : military_units) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} {}", military_unit->get_name(), military_unit->get_domain() == military_unit_domain::water ? "warship" : "regiment");
		}
	}

	const std::vector<const transporter_type *> transporters = get_enabled_transporters_for_culture(domain->get_game_data()->get_culture());
	if (!transporters.empty()) {
		for (const transporter_type *transporter : transporters) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} {}", transporter->get_name(), transporter->is_ship() ? "merchant ship" : "transporter");
		}
	}

	if (!this->get_enabled_government_types().empty()) {
		for (const government_type *government_type : this->get_enabled_government_types()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} government type", government_type->get_name());
		}
	}

	if (!this->get_enabled_laws().empty()) {
		for (const law *law : this->get_enabled_laws()) {
			if (law->get_conditions() != nullptr && !law->get_conditions()->check(domain, read_only_context(domain))) {
				continue;
			}

			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} law", law->get_name());
		}
	}

	const std::vector<const deity *> enabled_deities = this->get_enabled_deities_for_country(domain);
	if (!enabled_deities.empty()) {
		for (const deity *deity : enabled_deities) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} deity", deity->get_name());
		}
	}

	const std::vector<const deity *> disabled_deities = this->get_disabled_deities_for_country(domain);
	if (!disabled_deities.empty()) {
		for (const deity *deity : disabled_deities) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Obsoletes {} deity", deity->get_name());
		}
	}

	const std::vector<const employment_type *> &enabled_employment_types = this->get_enabled_employment_types();
	if (!enabled_employment_types.empty()) {
		for (const employment_type *employment_type : enabled_employment_types) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} employment type", employment_type->get_name());
		}
	}

	const std::vector<const item_type *> &enabled_item_types = this->get_enabled_item_types();
	if (!enabled_item_types.empty()) {
		for (const item_type *item_type : enabled_item_types) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} item type", item_type->get_name());
		}
	}

	const std::vector<const spell *> &enabled_spells = this->get_enabled_spells();
	if (!enabled_spells.empty()) {
		for (const spell *spell : enabled_spells) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} spell", spell->get_name());
		}
	}

	return str;
}

QString technology::get_effects_qstring(const domain *domain) const
{
	return QString::fromStdString(this->get_effects_string(domain));
}

bool technology::is_hidden_in_tree() const
{
	return !this->is_available_for_domain(game::get()->get_player_country());
}

bool technology::is_enabled() const
{
	if (this->required_game_rule != nullptr && game::get()->get_rules() != nullptr) {
		if (!game::get()->get_rules()->get_value(this->required_game_rule)) {
			return false;
		}
	}

	return true;
}

}
