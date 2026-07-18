#include "metternich.h"

#include "map/province_game_data.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/party.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "domain/domain_military.h"
#include "domain/domain_technology.h"
#include "domain/domain_turn_data.h"
#include "domain/government_type.h"
#include "economy/commodity.h"
#include "economy/commodity_container.h"
#include "economy/employment_type.h"
#include "economy/income_transaction_type.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/province_event.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_type.h"
#include "infrastructure/dungeon.h"
#include "infrastructure/holding_type.h"
#include "infrastructure/pathway.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_map_data.h"
#include "map/province_map_mode.h"
#include "map/province_turn_data.h"
#include "map/route.h"
#include "map/route_game_data.h"
#include "map/site.h"
#include "map/site_feature.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "religion/religion.h"
#include "script/context.h"
#include "script/modifier.h"
#include "script/scripted_province_modifier.h"
#include "technology/technology.h"
#include "technology/technology_category.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/civilian_unit.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/dice.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/vector_random_util.h"

#include "xbrz.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

province_game_data::province_game_data(const metternich::province *province)
	: province(province)
{
	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::type_size_changed, this, &province_game_data::on_population_type_size_changed);
	connect(this->get_population(), &population::main_culture_changed, this, &province_game_data::on_population_main_culture_changed);
	connect(this->get_population(), &population::main_religion_changed, this, &province_game_data::on_population_main_religion_changed);

	connect(this, &province_game_data::provincial_capital_changed, this, &province_game_data::visible_sites_changed);
	connect(this, &province_game_data::dungeon_sites_changed, this, &province_game_data::visible_sites_changed);

	connect(this, &province_game_data::level_changed, this, &province_game_data::income_changed);
}

province_game_data::~province_game_data()
{
}

void province_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "domain") {
		this->owner = domain::get(value);
	} else if (key == "culture") {
		this->culture = culture::get(value);
	} else if (key == "religion") {
		this->religion = religion::get(value);
	} else if (key == "trade_zone_domain") {
		this->trade_zone_domain = domain::get(value);
	} else if (key == "temple_domain") {
		this->temple_domain = domain::get(value);
	} else if (key == "level") {
		this->level = std::stoi(value);
	} else if (key == "max_level") {
		this->max_level = std::stoi(value);
	} else if (key == "provincial_capital") {
		this->provincial_capital = site::get(value);
	} else if (key == "pathway") {
		this->pathway = pathway::get(value);
	} else if (key == "under_construction_pathway") {
		this->under_construction_pathway = pathway::get(value);
	} else if (key == "pathway_construction_progress") {
		this->pathway_construction_progress = decimillesimal_int(value);
	} else if (key == "total_holding_level") {
		this->total_holding_level = std::stoi(value);
	} else if (key == "movement_cost_modifier") {
		this->movement_cost_modifier = std::stoi(value);
	} else {
		throw std::runtime_error(std::format("Invalid province game data property: \"{}\".", key));
	}
}

void province_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "technologies") {
		for (const std::string &value : values) {
			this->technologies.insert(technology::get(value));
		}
	} else if (tag == "site_feature_counts") {
		scope.for_each_property([this](const gsml_property &property) {
			this->site_feature_counts[site_feature::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "population_type_modifier_multipliers") {
		scope.for_each_property([this](const gsml_property &property) {
			this->population_type_modifier_multipliers[population_type::get(property.get_key())] = centesimal_int(property.get_value());
		});
	} else if (tag == "employment_capacity_modifiers") {
		scope.for_each_property([this](const gsml_property &property) {
			this->employment_capacity_modifiers[employment_type::get(property.get_key())] = std::stoll(property.get_value());
		});
	} else if (tag == "commodity_output_modifiers") {
		scope.for_each_property([this](const gsml_property &property) {
			this->commodity_output_modifiers[commodity::get(property.get_key())] = centesimal_int(property.get_value());
		});
	} else if (tag == "commodity_throughput_modifiers") {
		scope.for_each_property([this](const gsml_property &property) {
			this->commodity_throughput_modifiers[commodity::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "technology_category_spread_modifiers") {
		scope.for_each_property([this](const gsml_property &property) {
			this->technology_category_spread_modifiers[technology_category::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else {
		throw std::runtime_error(std::format("Invalid province game data scope: \"{}\".", tag));
	}
}

gsml_data province_game_data::to_gsml_data() const
{
	gsml_data data(this->province->get_identifier());

	if (this->get_owner() != nullptr) {
		data.add_property("domain", this->get_owner()->get_identifier());
	}

	if (this->get_culture() != nullptr) {
		data.add_property("culture", this->get_culture()->get_identifier());
	}

	if (this->get_religion() != nullptr) {
		data.add_property("religion", this->get_religion()->get_identifier());
	}

	if (this->get_trade_zone_domain() != nullptr) {
		data.add_property("trade_zone_domain", this->get_trade_zone_domain()->get_identifier());
	}

	if (this->get_temple_domain() != nullptr) {
		data.add_property("temple_domain", this->get_temple_domain()->get_identifier());
	}

	if (this->get_level() != 0) {
		data.add_property("level", std::to_string(this->get_level()));
	}

	if (this->get_max_level() != 0) {
		data.add_property("max_level", std::to_string(this->get_max_level()));
	}

	if (this->get_provincial_capital() != nullptr) {
		data.add_property("provincial_capital", this->get_provincial_capital()->get_identifier());
	}

	if (this->get_pathway() != nullptr) {
		data.add_property("pathway", this->get_pathway()->get_identifier());
	}

	if (this->get_under_construction_pathway() != nullptr) {
		data.add_property("under_construction_pathway", this->get_under_construction_pathway()->get_identifier());
	}

	if (this->get_pathway_construction_progress() != 0) {
		data.add_property("pathway_construction_progress", this->get_pathway_construction_progress().to_string());
	}

	if (this->get_total_holding_level() != 0) {
		data.add_property("total_holding_level", std::to_string(this->get_total_holding_level()));
	}

	if (this->get_movement_cost_modifier() != 0) {
		data.add_property("movement_cost_modifier", std::to_string(this->get_movement_cost_modifier()));
	}

	if (!this->get_site_feature_counts().empty()) {
		gsml_data site_feature_counts_data("site_feature_counts");
		for (const auto &[feature, count] : this->get_site_feature_counts()) {
			site_feature_counts_data.add_property(feature->get_identifier(), std::to_string(count));
		}
		data.add_child(std::move(site_feature_counts_data));
	}

	if (!this->get_technologies().empty()) {
		gsml_data technologies_data("technologies");
		for (const technology *technology : this->get_technologies()) {
			technologies_data.add_value(technology->get_identifier());
		}
		data.add_child(std::move(technologies_data));
	}

	if (!this->population_type_modifier_multipliers.empty()) {
		gsml_data population_type_modifier_multipliers_data("population_type_modifier_multipliers");
		for (const auto &[population_type, multiplier] : this->population_type_modifier_multipliers) {
			population_type_modifier_multipliers_data.add_property(population_type->get_identifier(), multiplier.to_string());
		}
		data.add_child(std::move(population_type_modifier_multipliers_data));
	}

	if (!this->employment_capacity_modifiers.empty()) {
		gsml_data employment_capacity_modifiers_data("employment_capacity_modifiers");
		for (const auto &[employment_type, modifier] : this->employment_capacity_modifiers) {
			employment_capacity_modifiers_data.add_property(employment_type->get_identifier(), std::to_string(modifier));
		}
		data.add_child(std::move(employment_capacity_modifiers_data));
	}

	if (!this->get_commodity_output_modifiers().empty()) {
		gsml_data commodity_output_modifiers_data("commodity_output_modifiers");
		for (const auto &[commodity, output_modifier] : this->get_commodity_output_modifiers()) {
			commodity_output_modifiers_data.add_property(commodity->get_identifier(), output_modifier.to_string());
		}
		data.add_child(std::move(commodity_output_modifiers_data));
	}

	if (!this->get_commodity_throughput_modifiers().empty()) {
		gsml_data commodity_throughput_modifiers_data("commodity_throughput_modifiers");
		for (const auto &[commodity, throughput_modifier] : this->get_commodity_throughput_modifiers()) {
			commodity_throughput_modifiers_data.add_property(commodity->get_identifier(), std::to_string(throughput_modifier));
		}
		data.add_child(std::move(commodity_throughput_modifiers_data));
	}

	if (!this->technology_category_spread_modifiers.empty()) {
		gsml_data technology_category_spread_modifiers_data("technology_category_spread_modifiers");
		for (const auto &[technology_category, modifier] : this->technology_category_spread_modifiers) {
			technology_category_spread_modifiers_data.add_property(technology_category->get_identifier(), std::to_string(modifier));
		}
		data.add_child(std::move(technology_category_spread_modifiers_data));
	}

	return data;
}

QCoro::Task<void> province_game_data::initialize()
{
	const terrain_type *terrain = this->get_terrain();
	if (terrain != nullptr && terrain->get_province_modifier() != nullptr) {
		co_await terrain->get_province_modifier()->apply(this->province);
	}
}

QCoro::Task<void> province_game_data::do_turn()
{
	for (const site *site : this->get_sites()) {
		assert_throw(site->get_map_data()->is_on_map());
		co_await site->get_game_data()->do_turn();
	}

	co_await this->decrement_scripted_modifiers();
}

QCoro::Task<void> province_game_data::do_events()
{
	const bool is_last_turn_of_year = game::get()->is_last_turn_of_year();
	if (is_last_turn_of_year) {
		co_await province_event::check_events_for_scope(this->province, event_trigger::yearly_pulse);
	}

	const bool is_last_turn_of_quarter = game::get()->is_last_turn_of_quarter();
	if (is_last_turn_of_quarter) {
		co_await province_event::check_events_for_scope(this->province, event_trigger::quarterly_pulse);
	}

	co_await province_event::check_events_for_scope(this->province, event_trigger::per_turn_pulse);
}

void province_game_data::do_ai_turn()
{
	//visit visitable sites (if any) with military units of this province's owner
	if (this->get_owner() != nullptr && this->has_domain_military_unit(this->get_owner())) {
		for (const site *site : this->get_sites()) {
			site_game_data *site_game_data = site->get_game_data();
			if (!site_game_data->can_be_visited_by(this->get_owner())) {
				continue;
			}

			std::vector<military_unit *> military_units = this->get_military_units();

			std::erase_if(military_units, [this](const military_unit *military_unit) {
				if (military_unit->get_country() != this->get_owner()) {
					return true;
				}

				if (military_unit->is_moving()) {
					return true;
				}

				if (military_unit->get_character() == nullptr) {
					return true;
				}

				return false;
			});

			if (!military_units.empty()) {
				//only visit if the character party has a suitable level
				const int max_appropriate_dungeon_level = party::get_max_appropriate_dungeon_level(army::get_characters(military_units));

				if (site_game_data->get_dungeon() == nullptr || site_game_data->get_dungeon()->get_level() <= max_appropriate_dungeon_level) {
					auto army = make_qunique<metternich::army>(military_units, site);
					this->get_owner()->get_military()->add_army(std::move(army));
				}
			}
			break;
		}
	}
}

void province_game_data::collect_taxes()
{
	assert_throw(this->get_owner() != nullptr);

	const dice &taxation_dice = defines::get()->get_province_taxation_for_level(this->get_level());
	const int64_t taxation = random::get()->roll_dice(taxation_dice) * defines::get()->get_domain_income_unit_value();
	if (taxation < 0) {
		//ignore negative results
		return;
	}

	this->get_owner()->get_economy()->add_tributable_commodity(defines::get()->get_wealth_commodity(), taxation, income_transaction_type::tribute);
	this->get_owner()->get_turn_data()->add_income_transaction(income_transaction_type::taxation, taxation, nullptr, 0, this->get_owner());
}

QCoro::Task<void> province_game_data::do_military_unit_recruitment()
{
	if (this->get_owner() == nullptr) {
		co_return;
	}

	try {
		if (this->get_owner()->get_game_data()->is_under_anarchy()) {
			co_return;
		}

		const military_unit_type_map<int> recruitment_counts = this->military_unit_recruitment_counts;
		if (recruitment_counts.empty()) {
			co_return;
		}

		for (const auto &[military_unit_type, recruitment_count] : recruitment_counts) {
			assert_throw(recruitment_count > 0);

			for (int i = 0; i < recruitment_count; ++i) {
				const bool created = co_await this->get_owner()->get_military()->create_military_unit(military_unit_type, this->province, nullptr, {});
				const bool restore_costs = !created;
				this->change_military_unit_recruitment_count(military_unit_type, -1, restore_costs);
			}
		}

		assert_throw(this->military_unit_recruitment_counts.empty());

		if (this->get_owner() == game::get()->get_player_country()) {
			std::string recruitment_counts_str;
			for (const auto &[military_unit_type, recruitment_count] : recruitment_counts) {
				recruitment_counts_str += std::format("\n{} {}", recruitment_count, military_unit_type->get_name());
			}

			const portrait *war_minister_portrait = this->get_owner()->get_government()->get_war_minister_portrait();

			engine_interface::get()->add_notification(std::format("Military Units Recruited in {}", this->get_current_cultural_name()), war_minister_portrait, std::format("{}, we have recruited new military units for our army in {}.\n{}", this->get_owner()->get_game_data()->get_form_of_address(), this->get_current_cultural_name(), recruitment_counts_str));
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing military unit recruitment for country \"{}\" in province \"{}\".", this->get_owner()->get_identifier(), this->province->get_identifier())));
	}
}

QCoro::Task<void> province_game_data::do_construction(const decimillesimal_int &construction_per_project)
{
	if (this->get_under_construction_pathway() == nullptr) {
		co_return;
	}

	const commodity_map<int64_t> commodity_costs = this->get_under_construction_pathway()->get_commodity_costs_for_province(this->province);
	if (commodity_costs.contains(defines::get()->get_construction_commodity())) {
		const decimillesimal_int construction_cost = decimillesimal_int(commodity_costs.find(defines::get()->get_construction_commodity())->second);
		this->change_pathway_construction_progress(construction_per_project * 100 / construction_cost);
	} else {
		this->change_pathway_construction_progress(decimillesimal_int(100));
	}

	if (this->get_pathway_construction_progress() >= 100) {
		const metternich::pathway *pathway = this->get_under_construction_pathway();
		co_await this->set_pathway(this->get_under_construction_pathway());
		this->set_under_construction_pathway(nullptr);
		emit this->get_owner()->get_game_data()->pathway_built(pathway, this->province);
	}
}

void province_game_data::do_population_literacy_change()
{
	if (this->get_population()->get_size() == 0) {
		return;
	}

	int64_t educator_size = 0;
	for (const auto &[population_type, population_type_size] : this->get_population()->get_type_sizes()) {
		if (population_type->is_educator()) {
			educator_size += population_type_size;
		}
	}

	const decimillesimal_int educator_percent = decimillesimal_int::min(decimillesimal_int(educator_size) * 100 / this->get_population()->get_size(), defines::get()->get_max_literacy_educator_percent() + defines::get()->get_base_literacy_educator_percent());

	const decimillesimal_int monthly_literacy_change_rate = (educator_percent - defines::get()->get_base_literacy_educator_percent()) * defines::get()->get_base_monthly_literacy_change_rate() / (defines::get()->get_max_literacy_educator_percent() - defines::get()->get_base_literacy_educator_percent());

	const decimillesimal_int literacy_change_rate = monthly_literacy_change_rate * game::get()->get_current_months_per_turn() / 100;

	for (population_unit *population_unit : this->get_population_units()) {
		population_unit->change_literacy_rate(literacy_change_rate);
	}
}

bool province_game_data::is_on_map() const
{
	return this->province->get_map_data()->is_on_map();
}

QCoro::Task<void> province_game_data::set_owner(const domain *domain)
{
	if (domain == this->get_owner()) {
		co_return;
	}

	const metternich::domain *old_owner = this->owner;

	this->owner = domain;

	for (const site *site : this->get_sites()) {
		if (site->get_game_data()->get_dungeon() != nullptr) {
			//dungeon sites cannot have owners
			continue;
		}

		if (site->get_game_data()->get_owner() == old_owner) {
			co_await site->get_game_data()->set_owner(domain);
		}
	}

	if (old_owner != nullptr) {
		co_await old_owner->get_game_data()->remove_province(this->province);
	}

	if (this->get_owner() != nullptr) {
		co_await this->get_owner()->get_game_data()->add_province(this->province);

		if (this->get_population()->get_main_culture() == nullptr) {
			this->set_culture(this->get_owner()->get_game_data()->get_culture());
		}

		if (this->get_population()->get_main_religion() == nullptr) {
			this->set_religion(this->get_owner()->get_game_data()->get_religion());
		}
	} else {
		//remove population if this province becomes unowned
		std::vector<population_unit *> population_units = this->population_units;
		for (population_unit *population_unit : population_units) {
			co_await population_unit->get_site()->get_game_data()->pop_population_unit(population_unit, true);
		}
	}

	//clear military unit recruitment if the owner changes
	this->clear_military_unit_recruitment_counts();

	if (game::get()->is_running()) {
		map::get()->update_minimap_rect(this->get_territory_rect());

		this->province->get_turn_data()->set_province_map_dirty(true);

		emit owner_changed();
	}
}

bool province_game_data::is_capital() const
{
	if (this->get_owner() == nullptr) {
		return false;
	}

	return this->get_owner()->get_game_data()->get_capital_province() == this->province;
}

void province_game_data::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	this->culture = culture;

	if (game::get()->is_running()) {
		this->province->get_turn_data()->set_province_map_mode_dirty(province_map_mode::cultural);

		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::cultural);
		}
	}

	emit culture_changed();

	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->can_have_population()) {
			site->get_game_data()->set_culture(culture);
		}
	}

	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->can_have_population()) {
			continue;
		}

		if (site->get_game_data()->get_population()->get_main_culture() == nullptr) {
			site->get_game_data()->set_culture(this->get_culture());
		}
	}
}

void province_game_data::on_population_main_culture_changed(const metternich::culture *culture)
{
	if (culture != nullptr) {
		this->set_culture(culture);
	} else if (this->get_owner() != nullptr) {
		this->set_culture(this->get_owner()->get_game_data()->get_culture());
	} else {
		this->set_culture(nullptr);
	}
}

void province_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->religion = religion;

	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->can_have_population()) {
			continue;
		}

		if (site->get_game_data()->get_population()->get_main_religion() == nullptr) {
			site->get_game_data()->set_religion(this->get_religion());
		}
	}

	if (game::get()->is_running()) {
		this->province->get_turn_data()->set_province_map_mode_dirty(province_map_mode::religious);

		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::religious);
		}
	}

	emit religion_changed();
}

void province_game_data::on_population_main_religion_changed(const metternich::religion *religion)
{
	if (religion != nullptr) {
		this->set_religion(religion);
	} else if (this->get_owner() != nullptr) {
		this->set_religion(this->get_owner()->get_game_data()->get_religion());
	} else {
		this->set_religion(nullptr);
	}
}

const std::string &province_game_data::get_current_cultural_name() const
{
	return this->province->get_cultural_name(this->get_culture());
}

void province_game_data::set_trade_zone_domain(const metternich::domain *trade_zone_domain)
{
	if (trade_zone_domain == this->get_trade_zone_domain()) {
		return;
	}

	this->trade_zone_domain = trade_zone_domain;

	if (game::get()->is_running()) {
		this->province->get_turn_data()->set_province_map_mode_dirty(province_map_mode::trade_zone);

		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::trade_zone);
		}
	}

	emit trade_zone_domain_changed();
}

void province_game_data::check_trade_zone_domain()
{
	if (this->province->is_water_zone()) {
		return;
	}

	domain_map<int> domain_economic_holding_levels;
	for (const site *holding_site : this->get_settlement_sites()) {
		const domain *holding_site_owner = holding_site->get_game_data()->get_owner();
		if (holding_site_owner == nullptr) {
			continue;
		}

		const holding_type *holding_type = holding_site->get_game_data()->get_holding_type();
		if (holding_type == nullptr || !holding_type->is_economic()) {
			continue;
		}

		if (!holding_site_owner->get_game_data()->get_government_type()->is_holding_type_allowed(holding_type)) {
			continue;
		}

		domain_economic_holding_levels[holding_site_owner] += holding_site->get_game_data()->get_holding_level();
	}

	if (domain_economic_holding_levels.empty()) {
		//if the province itself has no economic holdings, use those of neighboring provinces instead for this calculation

		for (const metternich::province *nearby_province : this->province->get_map_data()->get_nearby_provinces()) {
			for (const site *holding_site : nearby_province->get_game_data()->get_settlement_sites()) {
				const domain *holding_site_owner = holding_site->get_game_data()->get_owner();
				if (holding_site_owner == nullptr) {
					continue;
				}

				const holding_type *holding_type = holding_site->get_game_data()->get_holding_type();
				if (holding_type == nullptr || !holding_type->is_economic()) {
					continue;
				}

				if (!holding_site_owner->get_game_data()->get_government_type()->is_holding_type_allowed(holding_type)) {
					continue;
				}

				domain_economic_holding_levels[holding_site_owner] += holding_site->get_game_data()->get_holding_level();
			}
		}
	}

	int best_holding_level = -1;
	const domain *best_domain = nullptr;
	for (const auto &[domain, holding_level] : domain_economic_holding_levels) {
		if (holding_level > best_holding_level) {
			best_holding_level = holding_level;
			best_domain = domain;
		}
	}

	this->set_trade_zone_domain(best_domain);
}

void province_game_data::check_trade_zone_domain_for_province_and_neighbors()
{
	this->check_trade_zone_domain();

	for (const metternich::province *nearby_province : this->province->get_map_data()->get_nearby_provinces()) {
		nearby_province->get_game_data()->check_trade_zone_domain();
	}
}

void province_game_data::set_temple_domain(const metternich::domain *temple_domain)
{
	if (temple_domain == this->get_temple_domain()) {
		return;
	}

	this->temple_domain = temple_domain;

	if (game::get()->is_running()) {
		this->province->get_turn_data()->set_province_map_mode_dirty(province_map_mode::temple);

		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::temple);
		}
	}

	emit temple_domain_changed();
}

void province_game_data::check_temple_domain()
{
	domain_map<int> domain_religious_holding_levels;
	for (const site *holding_site : this->get_settlement_sites()) {
		const domain *holding_site_owner = holding_site->get_game_data()->get_owner();
		if (holding_site_owner == nullptr) {
			continue;
		}

		const holding_type *holding_type = holding_site->get_game_data()->get_holding_type();
		if (holding_type == nullptr || !holding_type->is_religious()) {
			continue;
		}

		if (!holding_site_owner->get_game_data()->get_government_type()->is_holding_type_allowed(holding_type)) {
			continue;
		}

		domain_religious_holding_levels[holding_site->get_game_data()->get_owner()] += holding_site->get_game_data()->get_holding_level();
	}

	if (domain_religious_holding_levels.empty()) {
		//if the province itself has no economic holdings, use those of neighboring provinces instead for this calculation

		for (const metternich::province *nearby_province : this->province->get_map_data()->get_nearby_provinces()) {
			for (const site *holding_site : nearby_province->get_game_data()->get_settlement_sites()) {
				const domain *holding_site_owner = holding_site->get_game_data()->get_owner();
				if (holding_site_owner == nullptr) {
					continue;
				}

				const holding_type *holding_type = holding_site->get_game_data()->get_holding_type();
				if (holding_type == nullptr || !holding_type->is_religious()) {
					continue;
				}

				if (!holding_site_owner->get_game_data()->get_government_type()->is_holding_type_allowed(holding_type)) {
					continue;
				}

				domain_religious_holding_levels[holding_site_owner] += holding_site->get_game_data()->get_holding_level();
			}
		}
	}

	int best_holding_level = -1;
	const domain *best_domain = nullptr;
	for (const auto &[domain, holding_level] : domain_religious_holding_levels) {
		if (holding_level > best_holding_level) {
			best_holding_level = holding_level;
			best_domain = domain;
		}
	}

	this->set_temple_domain(best_domain);
}

void province_game_data::check_temple_domain_for_province_and_neighbors()
{
	this->check_temple_domain();

	for (const metternich::province *nearby_province : this->province->get_map_data()->get_nearby_provinces()) {
		nearby_province->get_game_data()->check_temple_domain();
	}
}

void province_game_data::set_level(const int level)
{
	assert_throw(level >= 0);

	if (level == this->get_level()) {
		return;
	}

	if (level > this->get_max_level()) {
		this->set_level(this->get_max_level());
		return;
	}

	const int old_level = this->get_level();

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_score(-this->get_level() * 100);
		this->get_owner()->get_game_data()->change_domain_power(-this->get_level());
	}

	this->level = level;

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_score(this->get_level() * 100);
		this->get_owner()->get_game_data()->change_domain_power(this->get_level());
	}

	for (const site *holding_site : this->get_settlement_sites()) {
		if (!holding_site->get_game_data()->is_built()) {
			continue;
		}

		for (const qunique_ptr<building_slot> &building_slot : holding_site->get_game_data()->get_building_slots()) {
			const building_type *building = building_slot->get_building();

			if (building == nullptr || building->get_holding_level() == 0) {
				continue;
			}

			const int64_t old_population_capacity = building->get_population_capacity_for_province_level(old_level);
			const int64_t new_population_capacity = building->get_population_capacity_for_province_level(level);
			holding_site->get_game_data()->change_population_capacity(new_population_capacity - old_population_capacity);
		}

		emit holding_site->get_game_data()->income_changed();
	}

	if (game::get()->is_running()) {
		emit level_changed();

		if (this->get_owner() != nullptr) {
			emit this->get_owner()->get_game_data()->income_changed();
		}
	}
}

void province_game_data::change_level(const int change)
{
	this->set_level(this->get_level() + change);
}

int province_game_data::get_max_level() const
{
	return this->max_level;
}

void province_game_data::set_max_level(const int level)
{
	assert_throw(level >= 0);

	if (level == this->get_max_level()) {
		return;
	}

	this->max_level = level;

	if (this->get_max_level() < this->get_level()) {
		this->set_level(this->get_max_level());
	}

	if (game::get()->is_running()) {
		emit max_level_changed();
	}
}

void province_game_data::change_max_level(const int change)
{
	this->set_max_level(this->get_max_level() + change);
}

bool province_game_data::is_coastal() const
{
	return this->province->get_map_data()->is_coastal();
}

bool province_game_data::is_near_water() const
{
	return this->province->get_map_data()->is_near_water();
}

const QRect &province_game_data::get_territory_rect() const
{
	return this->province->get_map_data()->get_territory_rect();
}

const QPoint &province_game_data::get_territory_rect_center() const
{
	return this->province->get_map_data()->get_territory_rect_center();
}

const std::vector<const metternich::province *> &province_game_data::get_neighbor_provinces() const
{
	return this->province->get_map_data()->get_neighbor_provinces();
}

bool province_game_data::is_country_border_province() const
{
	for (const metternich::province *neighbor_province : this->get_neighbor_provinces()) {
		const province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->get_owner()) {
			return true;
		}
	}

	return false;
}

const site *province_game_data::get_provincial_capital() const
{
	return this->provincial_capital;
}

void province_game_data::set_provincial_capital(const site *site)
{
	if (site == this->get_provincial_capital()) {
		return;
	}

	this->provincial_capital = site;

	if (game::get()->is_running()) {
		emit provincial_capital_changed();
	}
}

void province_game_data::choose_provincial_capital()
{
	if (this->get_owner() == nullptr) {
		return;
	}

	const government_type *government_type = this->get_owner()->get_game_data()->get_government_type();
	std::vector<const site *> potential_provincial_capitals;
	bool found_default_provincial_capital = false;
	bool found_preferred_holding_type = false;
	int best_holding_level = 0;

	for (const site *site : this->province->get_map_data()->get_settlement_sites()) {
		if (!site->get_game_data()->is_built()) {
			continue;
		}

		if (site->get_game_data()->get_owner() != this->get_owner()) {
			continue;
		}

		if (!government_type->is_holding_type_allowed(site->get_game_data()->get_holding_type())) {
			continue;
		}

		if (this->get_owner() != nullptr && site == this->get_owner()->get_default_capital()) {
			potential_provincial_capitals = { site };
			break;
		} else if (this->get_owner() != nullptr && site == this->get_owner()->get_game_data()->get_capital()) {
			potential_provincial_capitals = { site };
			break;
		} else if (site == this->province->get_default_provincial_capital()) {
			potential_provincial_capitals = { site };
			found_default_provincial_capital = true;
		} else if (!found_default_provincial_capital) {
			if (found_preferred_holding_type) {
				if (!government_type->is_holding_type_preferred(site->get_game_data()->get_holding_type())) {
					continue;
				}
			} else if (government_type->is_holding_type_preferred(site->get_game_data()->get_holding_type())) {
				potential_provincial_capitals.clear();
				found_preferred_holding_type = true;
				best_holding_level = site->get_game_data()->get_holding_level();
			}

			if (site->get_game_data()->get_holding_level() > best_holding_level) {
				potential_provincial_capitals.clear();
				best_holding_level = site->get_game_data()->get_holding_level();
			} else if (site->get_game_data()->get_holding_level() < best_holding_level) {
				continue;
			}

			potential_provincial_capitals.push_back(site);
		}
	}

	if (!potential_provincial_capitals.empty()) {
		this->set_provincial_capital(vector::get_random(potential_provincial_capitals));
	} else {
		this->set_provincial_capital(nullptr);
	}
}

const site *province_game_data::get_best_provincial_capital_slot() const
{
	if (this->get_owner() == nullptr) {
		return nullptr;
	}

	assert_throw(this->get_provincial_capital() == nullptr);

	std::vector<const site *> potential_provincial_capitals;
	bool found_default_provincial_capital = false;
	int best_holding_level = 0;

	for (const site *site : this->province->get_map_data()->get_settlement_sites()) {
		if (site->get_game_data()->is_built()) {
			continue;
		}

		if (site->get_holding_type() == nullptr) {
			continue;
		}

		if (site->get_game_data()->get_owner() != this->get_owner()) {
			continue;
		}

		if (!this->get_owner()->get_game_data()->get_government_type()->is_holding_type_allowed(site->get_holding_type())) {
			continue;
		}

		if (this->get_owner() != nullptr && site == this->get_owner()->get_default_capital()) {
			potential_provincial_capitals = { site };
			break;
		} else if (site == this->province->get_default_provincial_capital()) {
			potential_provincial_capitals = { site };
			found_default_provincial_capital = true;
		} else if (!found_default_provincial_capital) {
			if (site->get_game_data()->get_holding_level() > best_holding_level) {
				potential_provincial_capitals.clear();
				best_holding_level = site->get_game_data()->get_holding_level();
			} else if (site->get_game_data()->get_holding_level() < best_holding_level) {
				continue;
			}

			potential_provincial_capitals.push_back(site);
		}
	}

	if (!potential_provincial_capitals.empty()) {
		return vector::get_random(potential_provincial_capitals);
	} else {
		return nullptr;
	}
}

const QPoint &province_game_data::get_center_tile_pos() const
{
	return this->province->get_map_data()->get_center_tile_pos();
}

QCoro::Task<void> province_game_data::set_pathway(const metternich::pathway *pathway)
{
	if (pathway == this->get_pathway()) {
		co_return;
	}

	if (this->get_pathway() != nullptr && this->get_pathway()->get_modifier() != nullptr) {
		co_await this->get_pathway()->get_modifier()->remove(this->province);
	}

	this->pathway = pathway;

	if (this->get_pathway() != nullptr && this->get_pathway()->get_modifier() != nullptr) {
		co_await this->get_pathway()->get_modifier()->apply(this->province);
	}

	if (game::get()->is_running()) {
		for (const route *route : this->province->get_routes()) {
			route->get_game_data()->check_active();
		}

		emit pathway_changed();
	}
}

void province_game_data::set_under_construction_pathway(const metternich::pathway *pathway)
{
	if (pathway == this->get_under_construction_pathway()) {
		return;
	}

	this->under_construction_pathway = pathway;

	this->pathway_construction_progress = decimillesimal_int(0);

	if (game::get()->is_running()) {
		emit under_construction_pathway_changed();
	}
}

bool province_game_data::has_pathway_or_better(const metternich::pathway *pathway) const
{
	assert_throw(pathway != nullptr);

	if (this->get_pathway() == pathway) {
		return true;
	}

	if (this->get_pathway() == nullptr) {
		return false;
	}

	return this->get_pathway()->get_transport_level() >= pathway->get_transport_level();
}

bool province_game_data::can_build_pathway(const metternich::pathway *pathway) const
{
	if (!pathway->is_buildable_in_province(this->province)) {
		return false;
	}

	if (this->get_owner() == nullptr) {
		return false;
	}

	const domain_economy *domain_economy = this->get_owner()->get_economy();

	for (const auto &[commodity, cost] : pathway->get_commodity_costs_for_province(this->province)) {
		if (commodity == defines::get()->get_construction_commodity()) {
			continue;
		}

		if (cost > domain_economy->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return true;
}

void province_game_data::build_pathway(const metternich::pathway *pathway)
{
	if (this->get_owner() == nullptr) {
		return;
	}

	if (this->get_under_construction_pathway() != nullptr) {
		this->cancel_pathway_construction();
	}

	domain_economy *domain_economy = this->get_owner()->get_economy();

	for (const auto &[commodity, cost] : pathway->get_commodity_costs_for_province(this->province)) {
		if (commodity == defines::get()->get_construction_commodity()) {
			continue;
		}

		domain_economy->change_stored_commodity(commodity, -cost);
	}

	this->set_under_construction_pathway(pathway);

	if (this->get_owner()->get_game_data()->get_construction_chosen_promise() != nullptr) {
		this->get_owner()->get_game_data()->get_construction_chosen_promise()->finish();
	}
}

void province_game_data::cancel_pathway_construction()
{
	if (this->get_under_construction_pathway() == nullptr) {
		return;
	}

	if (this->get_owner() != nullptr) {
		domain_economy *domain_economy = this->get_owner()->get_economy();

		for (const auto &[commodity, cost] : this->get_under_construction_pathway()->get_commodity_costs_for_province(this->province)) {
			if (commodity == defines::get()->get_construction_commodity()) {
				continue;
			}

			domain_economy->change_stored_commodity(commodity, cost);
		}
	}

	this->set_under_construction_pathway(nullptr);
}

const pathway *province_game_data::get_buildable_pathway() const
{
	for (const metternich::pathway *pathway : pathway::get_all()) {
		if (!this->can_build_pathway(pathway)) {
			continue;
		}

		return pathway;
	}

	return nullptr;
}

const decimillesimal_int &province_game_data::get_pathway_construction_progress() const
{
	return this->pathway_construction_progress;
}

qint64 province_game_data::get_pathway_construction_progress_commodity_quantity() const
{
	const commodity_map<int64_t> commodity_costs = this->get_under_construction_pathway()->get_commodity_costs_for_province(this->province);
	if (!commodity_costs.contains(defines::get()->get_construction_commodity())) {
		return 0;
	}

	return (commodity_costs.find(defines::get()->get_construction_commodity())->second * this->get_pathway_construction_progress() / 100).to_int64();
}

QString province_game_data::get_pathway_construction_progress_qstring() const
{
	return QString::fromStdString(std::to_string(this->get_pathway_construction_progress().to_int()));
}

void province_game_data::change_pathway_construction_progress(const decimillesimal_int &change)
{
	if (change == 0) {
		return;
	}

	this->pathway_construction_progress += change;
	assert_throw(this->pathway_construction_progress >= 0);
}

const std::vector<QPoint> &province_game_data::get_resource_tiles() const
{
	return this->province->get_map_data()->get_resource_tiles();
}

const std::vector<const site *> &province_game_data::get_sites() const
{
	return this->province->get_map_data()->get_sites();
}

const std::vector<const site *> &province_game_data::get_settlement_sites() const
{
	return this->province->get_map_data()->get_settlement_sites();
}

const QColor &province_game_data::get_map_color() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_game_data()->get_diplomatic_map_color();
	}

	if (this->province->is_water_zone()) {
		return defines::get()->get_ocean_color();
	} else {
		return defines::get()->get_map_blank_color();
	}
}

QImage province_game_data::prepare_map_image() const
{
	assert_throw(this->province->get_map_data()->get_territory_rect().width() > 0);
	assert_throw(this->province->get_map_data()->get_territory_rect().height() > 0);

	QImage image(this->province->get_map_data()->get_territory_rect().size(), QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	return image;
}

QImage province_game_data::finalize_map_image(QImage &&image)
{
	assert_throw(!image.isNull());

	const int tile_scale = defines::get()->get_province_map_tile_scale();

	if (tile_scale > 1) {
		QImage scaled_image;

		scaled_image = image::scale<QImage::Format_ARGB32>(image, centesimal_int(tile_scale), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});

		image = std::move(scaled_image);
	}

	std::vector<QPoint> border_pixels;

	for (int x = 0; x < image.width(); ++x) {
		for (int y = 0; y < image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (image.width() - 1) || pixel_pos.y() == (image.height() - 1)) {
				border_pixels.push_back(pixel_pos);
				continue;
			}

			if (pixel_color.alpha() != 255) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			const QPoint north_pos = pixel_pos + QPoint(0, -1);
			const QPoint east_pos = pixel_pos + QPoint(1, 0);
			const bool is_border_pixel = image.pixelColor(north_pos).alpha() == 0 || image.pixelColor(east_pos).alpha() == 0;

			if (is_border_pixel) {
				border_pixels.push_back(pixel_pos);
			}
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : border_pixels) {
		image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	return image;
}

void province_game_data::create_map_image()
{
	const map *map = map::get();

	QImage diplomatic_map_image = this->prepare_map_image();
	QImage selected_map_image = diplomatic_map_image;
	QImage interactive_map_image = diplomatic_map_image;

	const QColor &color = this->get_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();
	static const QColor interactive_color = QColor(Qt::darkGreen);

	for (int x = 0; x < this->province->get_map_data()->get_territory_rect().width(); ++x) {
		for (int y = 0; y < this->province->get_map_data()->get_territory_rect().height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->province->get_map_data()->get_territory_rect().topLeft() + relative_tile_pos);

			if (tile->get_province() != this->province) {
				continue;
			}

			diplomatic_map_image.setPixelColor(relative_tile_pos, color);
			selected_map_image.setPixelColor(relative_tile_pos, selected_color);
			interactive_map_image.setPixelColor(relative_tile_pos, interactive_color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->map_image_promise = promise;
	this->map_image_promise->start();
	assert_throw(!diplomatic_map_image.isNull());
	QThreadPool::globalInstance()->start([promise, image = std::move(diplomatic_map_image)]() mutable {
		promise->addResult(province_game_data::finalize_map_image(std::move(image)));
		promise->finish();
	});

	std::shared_ptr<QPromise<QImage>> selected_promise = std::make_shared<QPromise<QImage>>();
	this->selected_map_image_promise = selected_promise;
	this->selected_map_image_promise->start();
	assert_throw(!selected_map_image.isNull());
	QThreadPool::globalInstance()->start([selected_promise, image = std::move(selected_map_image)]() mutable {
		selected_promise->addResult(province_game_data::finalize_map_image(std::move(image)));
		selected_promise->finish();
	});

	std::shared_ptr<QPromise<QImage>> interactive_promise = std::make_shared<QPromise<QImage>>();
	this->interactive_map_image_promise = interactive_promise;
	this->interactive_map_image_promise->start();
	assert_throw(!interactive_map_image.isNull());
	QThreadPool::globalInstance()->start([interactive_promise, image = std::move(interactive_map_image)]() mutable {
		interactive_promise->addResult(province_game_data::finalize_map_image(std::move(image)));
		interactive_promise->finish();
	});

	const int tile_scale = defines::get()->get_province_map_tile_scale();
	this->map_image_rect = QRect(this->province->get_map_data()->get_territory_rect().topLeft() * tile_scale, this->province->get_map_data()->get_territory_rect().size() * tile_scale);

	this->create_map_mode_image(province_map_mode::terrain);

	if (!this->province->is_water_zone()) {
		this->create_map_mode_image(province_map_mode::cultural);
		this->create_map_mode_image(province_map_mode::religious);
		this->create_map_mode_image(province_map_mode::technology);
		this->create_map_mode_image(province_map_mode::trade_zone);
		this->create_map_mode_image(province_map_mode::temple);
	}

	this->calculate_text_rect();

	emit map_image_changed();
}

const QPromise<QImage> *province_game_data::get_map_mode_image_promise(const province_map_mode mode) const
{
	if (this->province->is_water_zone() && mode != province_map_mode::terrain) {
		return this->get_map_image_promise();
	}

	const auto find_iterator = this->map_mode_image_promises.find(mode);
	if (find_iterator != this->map_mode_image_promises.end()) {
		return find_iterator->second.get();
	}

	throw std::runtime_error(std::format("No map image promise found for mode {}.", magic_enum::enum_name(mode)));
}

void province_game_data::create_map_mode_image(const province_map_mode mode)
{
	assert_throw(!this->province->is_water_zone() || mode == province_map_mode::terrain);

	static const QColor empty_color(Qt::black);

	const map *map = map::get();

	QImage image = this->prepare_map_image();

	QColor province_color = this->get_map_color();
	switch (mode) {
		case province_map_mode::terrain:
			province_color = this->get_terrain()->get_color();
			break;
		case province_map_mode::cultural: {
			const metternich::culture *culture = this->get_culture();
			if (culture != nullptr) {
				province_color = culture->get_color();
			}
			break;
		}
		case province_map_mode::religious: {
			const metternich::religion *religion = this->get_religion();
			if (religion != nullptr) {
				province_color = religion->get_color();
			}
			break;
		}
		case province_map_mode::technology: {
			if (!this->province->is_water_zone()) {
				const int province_technology_count = static_cast<int>(this->get_technologies().size());
				const int total_technology_count = static_cast<int>(technology::get_all().size());
				const QColor &min_technology_color = defines::get()->get_map_blank_color();
				static const QColor max_technology_color(Qt::darkBlue);

				assert_throw(min_technology_color.red() >= max_technology_color.red());
				assert_throw(min_technology_color.green() >= max_technology_color.green());
				assert_throw(min_technology_color.blue() >= max_technology_color.blue());

				province_color.setRed(max_technology_color.red() + (min_technology_color.red() - max_technology_color.red()) * (total_technology_count - province_technology_count) / total_technology_count);
				province_color.setGreen(max_technology_color.green() + (min_technology_color.green() - max_technology_color.green()) * (total_technology_count - province_technology_count) / total_technology_count);
				province_color.setBlue(max_technology_color.blue() + (min_technology_color.blue() - max_technology_color.blue()) * (total_technology_count - province_technology_count) / total_technology_count);
				break;
			}
		}
		case province_map_mode::trade_zone: {
			if (!this->province->is_water_zone()) {
				const domain *trade_zone_domain = this->get_trade_zone_domain();
				if (trade_zone_domain != nullptr) {
					province_color = trade_zone_domain->get_color();
				} else {
					province_color = defines::get()->get_map_blank_color();
				}
			}
			break;
		}
		case province_map_mode::temple: {
			if (!this->province->is_water_zone()) {
				const domain *temple_domain = this->get_temple_domain();
				if (temple_domain != nullptr) {
					province_color = temple_domain->get_color();
				} else {
					province_color = defines::get()->get_map_blank_color();
				}
			}
			break;
		}
		default:
			break;
	}

	for (int x = 0; x < this->get_territory_rect().width(); ++x) {
		for (int y = 0; y < this->get_territory_rect().height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->get_territory_rect().topLeft() + relative_tile_pos);

			if (tile->get_province() != this->province) {
				continue;
			}

			const QColor *color = nullptr;

			switch (mode) {
				case province_map_mode::terrain:
				case province_map_mode::cultural:
				case province_map_mode::religious:
				case province_map_mode::technology:
				case province_map_mode::trade_zone:
				case province_map_mode::temple:
					color = &province_color;
					break;
				default:
					assert_throw(false);
					break;
			}

			assert_throw(color != nullptr);

			image.setPixelColor(relative_tile_pos, *color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->map_mode_image_promises[mode] = promise;
	promise->start();

	QThreadPool::globalInstance()->start([promise, image = std::move(image)]() mutable {
		promise->addResult(province_game_data::finalize_map_image(std::move(image)));
		promise->finish();
	});

	emit map_mode_image_changed(QString::fromUtf8(magic_enum::enum_name(mode)));
}

void province_game_data::calculate_text_rect()
{
	this->text_rect = QRect();

	const QPoint center_pos = this->get_center_tile_pos();

	const map *map = map::get();

	assert_throw(map->get_tile(center_pos)->get_province() == this->province);

	this->text_rect = QRect(center_pos, QSize(1, 1));

	bool changed = true;
	while (changed) {
		changed = false;

		bool can_expand_left = true;
		const int left_x = this->text_rect.left() - 1;
		for (int y = this->text_rect.top(); y <= this->text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(left_x, y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_left = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_left = false;
				break;
			}
		}
		if (can_expand_left) {
			this->text_rect.setLeft(left_x);
			changed = true;
		}

		bool can_expand_right = true;
		const int right_x = this->text_rect.right() + 1;
		for (int y = this->text_rect.top(); y <= this->text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(right_x, y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_right = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_right = false;
				break;
			}
		}
		if (can_expand_right) {
			this->text_rect.setRight(right_x);
			changed = true;
		}

		bool can_expand_up = true;
		const int up_y = this->text_rect.top() - 1;
		for (int x = this->text_rect.left(); x <= this->text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, up_y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_up = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_up = false;
				break;
			}
		}
		if (can_expand_up) {
			this->text_rect.setTop(up_y);
			changed = true;
		}

		bool can_expand_down = true;
		const int down_y = this->text_rect.bottom() + 1;
		for (int x = this->text_rect.left(); x <= this->text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, down_y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_down = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_down = false;
				break;
			}
		}
		if (can_expand_down) {
			this->text_rect.setBottom(down_y);
			changed = true;
		}
	}
}

std::vector<const site *> province_game_data::get_visible_sites() const
{
	std::vector<const site *> visible_sites = this->province->get_map_data()->get_sites();

	std::erase_if(visible_sites, [](const site *site) {
		if (site->get_type() == site_type::holding || (site->get_type() == site_type::dungeon && site->get_game_data()->get_dungeon() != nullptr)) {
			return false;
		}

		return !site->get_game_data()->is_built();
	});

	std::sort(visible_sites.begin(), visible_sites.end(), [](const site *lhs, const site *rhs) {
		if (lhs->get_game_data()->is_provincial_capital() != rhs->get_game_data()->is_provincial_capital()) {
			return lhs->get_game_data()->is_provincial_capital();
		}

		if (lhs->get_game_data()->is_used() != rhs->get_game_data()->is_used()) {
			return lhs->get_game_data()->is_used();
		}

		if (lhs->get_type() != rhs->get_type()) {
			return lhs->get_type() < rhs->get_type();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	return visible_sites;
}

QVariantList province_game_data::get_visible_sites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_visible_sites());
}

QVariantList province_game_data::get_dungeon_sites_qvariant_list() const
{
	std::vector<const site *> dungeon_sites;

	for (const site *site : this->province->get_map_data()->get_sites()) {
		if (site->get_type() == site_type::dungeon && site->get_game_data()->get_dungeon() != nullptr) {
			dungeon_sites.push_back(site);
		}
	}

	return container::to_qvariant_list(dungeon_sites);
}

const resource_map<int> &province_game_data::get_resource_counts() const
{
	return this->province->get_map_data()->get_resource_counts();
}

const terrain_type *province_game_data::get_terrain() const
{
	return this->province->get_map_data()->get_terrain();
}

bool province_game_data::produces_commodity(const commodity *commodity) const
{
	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->produces_commodity(commodity)) {
			return true;
		}
	}

	for (const site *site : this->province->get_map_data()->get_settlement_sites()) {
		if (!site->get_game_data()->is_built()) {
			continue;
		}

		if (site->get_game_data()->produces_commodity(commodity)) {
			return true;
		}
	}

	return false;
}

QVariantList province_game_data::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

QCoro::Task<void> province_game_data::add_technology(const technology *technology)
{
	if (this->has_technology(technology)) {
		co_return;
	}

	this->technologies.insert(technology);

	co_await this->on_technology_gained(technology, 1);

	for (const site *holding_site : this->get_settlement_sites()) {
		if (holding_site->get_game_data()->is_capital()) {
			co_await holding_site->get_game_data()->get_owner()->get_technology()->on_technology_added(technology);
		}
	}

	if (technology->get_discovery_event() != nullptr) {
		//ensure that a technology's discovery event is in the list of fired events if someone has that technology
		game::get()->add_fired_event(technology->get_discovery_event());
	}

	if (game::get()->is_running()) {
		this->province->get_turn_data()->set_province_map_mode_dirty(province_map_mode::technology);

		emit technologies_changed();
	}
}

QCoro::Task<void> province_game_data::add_technology_with_prerequisites(const technology *technology)
{
	co_await this->add_technology(technology);

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		co_await this->add_technology_with_prerequisites(prerequisite);
	}
}

QCoro::Task<void> province_game_data::remove_technology(const technology *technology)
{
	assert_throw(technology != nullptr);

	if (!this->has_technology(technology)) {
		co_return;
	}

	this->technologies.erase(technology);

	co_await this->on_technology_gained(technology, -1);

	for (const site *holding_site : this->get_settlement_sites()) {
		if (holding_site->get_game_data()->is_capital()) {
			co_await holding_site->get_game_data()->get_owner()->get_technology()->on_technology_lost(technology);
		}
	}

	//remove any technologies requiring this one as well
	for (const metternich::technology *requiring_technology : technology->get_leads_to()) {
		co_await this->remove_technology(requiring_technology);
	}

	if (game::get()->is_running()) {
		this->province->get_turn_data()->set_province_map_mode_dirty(province_map_mode::technology);

		emit technologies_changed();
	}
}

bool province_game_data::can_gain_technology(const technology *technology) const
{
	assert_throw(technology != nullptr);

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

QCoro::Task<void> province_game_data::on_technology_gained(const technology *technology, const int multiplier)
{
	if (technology->get_modifier() != nullptr) {
		co_await technology->get_modifier()->apply(this->province, multiplier);
	}
}

centesimal_int province_game_data::get_extra_technology(const technology *technology) const
{
	centesimal_int extra_technology;

	if (!this->has_technology(technology)) {
		return extra_technology;
	}

	//check technologies which have this one as their prerequisite
	for (const metternich::technology *requiring_technology : technology->get_leads_to()) {
		if (!this->has_technology(requiring_technology)) {
			continue;
		}

		extra_technology = centesimal_int::max(this->get_extra_technology(requiring_technology) + 1, extra_technology);
	}

	return extra_technology;
}

QVariantList province_game_data::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

bool province_game_data::has_scripted_modifier(const scripted_province_modifier *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

QCoro::Task<void> province_game_data::add_scripted_modifier(const scripted_province_modifier *modifier, const int duration)
{
	const read_only_context ctx(this->province);

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (modifier->get_modifier() != nullptr) {
		co_await this->apply_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

QCoro::Task<void> province_game_data::remove_scripted_modifier(const scripted_province_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		co_await this->remove_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

QCoro::Task<void> province_game_data::decrement_scripted_modifiers()
{
	std::vector<const scripted_province_modifier *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_province_modifier *modifier : modifiers_to_remove) {
		co_await this->remove_scripted_modifier(modifier);
	}
}

QCoro::Task<void> province_game_data::apply_modifier(const modifier<const metternich::province> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	co_await modifier->apply(this->province, multiplier);
}

void province_game_data::add_population_unit(population_unit *population_unit)
{
	this->population_units.push_back(population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void province_game_data::remove_population_unit(population_unit *population_unit)
{
	std::erase(this->population_units, population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void province_game_data::clear_population_units()
{
	this->population_units.clear();
}

QCoro::Task<void> province_game_data::on_population_type_size_changed(const population_type *population_type, const int64_t change)
{
	if (population_type->get_province_modifier() != nullptr) {
		const int64_t new_population_type_size = this->get_population()->get_type_size(population_type);
		const int64_t old_population_type_size = new_population_type_size - change;
		const centesimal_int population_type_modifier_multiplier = this->get_population_type_modifier_multiplier(population_type);

		const centesimal_int old_multiplier = centesimal_int::min(centesimal_int(old_population_type_size) / population_type->get_base_modifier_population_size() * population_type_modifier_multiplier, population_type->get_max_modifier_multiplier());
		const centesimal_int new_multiplier = centesimal_int::min(centesimal_int(new_population_type_size) / population_type->get_base_modifier_population_size() * population_type_modifier_multiplier, population_type->get_max_modifier_multiplier());

		co_await population_type->get_province_modifier()->apply(this->province,  -old_multiplier);
		co_await population_type->get_province_modifier()->apply(this->province, new_multiplier);
	}
}


QCoro::Task<void> province_game_data::set_population_type_modifier_multiplier(const population_type *population_type, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_population_type_modifier_multiplier(population_type);

	if (value == old_value) {
		co_return;
	}

	assert_throw(population_type->get_province_modifier() != nullptr);

	if (value == 1) {
		this->population_type_modifier_multipliers.erase(population_type);
	} else {
		this->population_type_modifier_multipliers[population_type] = value;
	}

	const int64_t population_type_size = this->get_population()->get_type_size(population_type);

	const centesimal_int old_multiplier = centesimal_int::min(centesimal_int(population_type_size) / population_type->get_base_modifier_population_size() * old_value, population_type->get_max_modifier_multiplier());
	const centesimal_int new_multiplier = centesimal_int::min(centesimal_int(population_type_size) / population_type->get_base_modifier_population_size() * value, population_type->get_max_modifier_multiplier());

	co_await population_type->get_province_modifier()->apply(this->province, -old_multiplier);
	co_await population_type->get_province_modifier()->apply(this->province, new_multiplier);
}

void province_game_data::set_employment_capacity_modifier(const employment_type *employment_type, const int64_t modifier)
{
	assert_throw(employment_type != nullptr);

	if (modifier == this->get_employment_capacity_modifier(employment_type)) {
		return;
	}

	if (modifier == 0) {
		this->employment_capacity_modifiers.erase(employment_type);
	} else {
		this->employment_capacity_modifiers[employment_type] = modifier;
	}

	//recalculate the affected employment capacity for holdings in this province
	for (const site *site : this->get_settlement_sites()) {
		site->get_game_data()->calculate_employment_capacity(employment_type);
	}
}

QVariantList province_game_data::get_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_units());
}

std::vector<military_unit *> province_game_data::get_domain_military_units(const domain *domain) const
{
	std::vector<military_unit *> domain_military_units = this->get_military_units();

	std::erase_if(domain_military_units, [domain](const military_unit *military_unit) {
		return military_unit->get_country() != domain;
	});

	return domain_military_units;
}

QVariantList province_game_data::get_domain_military_units_qvariant_list(const domain *domain) const
{
	return container::to_qvariant_list(this->get_domain_military_units(domain));
}

void province_game_data::add_military_unit(military_unit *military_unit)
{
	this->military_units.push_back(military_unit);

	if (!military_unit->is_moving()) {
		this->change_military_unit_category_count(military_unit->get_category(), 1);
	}

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void province_game_data::remove_military_unit(military_unit *military_unit)
{
	std::erase(this->military_units, military_unit);

	if (!military_unit->is_moving()) {
		this->change_military_unit_category_count(military_unit->get_category(), -1);
	}

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void province_game_data::clear_military_units()
{
	this->military_units.clear();
	this->military_unit_category_counts.clear();
}

QVariantList province_game_data::get_military_unit_category_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->military_unit_category_counts);
}

void province_game_data::change_military_unit_category_count(const military_unit_category category, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->military_unit_category_counts[category] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->military_unit_category_counts.erase(category);
	}

	if (game::get()->is_running()) {
		emit military_unit_category_counts_changed();
	}
}

bool province_game_data::has_domain_military_unit(const domain *domain) const
{
	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == domain) {
			return true;
		}
	}

	return false;
}

QVariantList province_game_data::get_domain_military_unit_category_counts(metternich::domain *domain) const
{
	std::map<military_unit_category, int> counts;

	for (const military_unit *military_unit : this->get_domain_military_units(domain)) {
		if (!military_unit->is_moving()) {
			++counts[military_unit->get_category()];
		}
	}

	return archimedes::map::to_qvariant_list(counts);
}

int province_game_data::get_domain_military_unit_category_count(const metternich::military_unit_category category, metternich::domain *domain) const
{
	int count = 0;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_category() == category && military_unit->get_country() == domain && !military_unit->is_moving()) {
			++count;
		}
	}

	return count;
}

const icon *province_game_data::get_military_unit_icon() const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (!military_unit->is_moving()) {
			++icon_counts[military_unit->get_icon()];
		}
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	assert_throw(best_icon != nullptr);

	return best_icon;
}

const icon *province_game_data::get_military_unit_category_icon(const military_unit_category category) const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->military_units) {
		if (military_unit->get_category() != category) {
			continue;
		}

		++icon_counts[military_unit->get_icon()];
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	assert_throw(best_icon != nullptr);

	return best_icon;
}

QString province_game_data::get_military_unit_category_name(const military_unit_category category) const
{
	std::map<QString, int> name_counts;

	for (const military_unit *military_unit : this->military_units) {
		if (military_unit->get_category() != category) {
			continue;
		}

		++name_counts[military_unit->get_type()->get_name_qstring()];
	}

	QString best_name;
	int best_name_count = 0;
	for (const auto &[name, count] : name_counts) {
		if (count > best_name_count) {
			best_name = name;
			best_name_count = count;
		}
	}

	assert_throw(!best_name.isEmpty());

	return best_name;
}

const icon *province_game_data::get_domain_military_unit_icon(metternich::domain *domain) const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == domain && !military_unit->is_moving()) {
			++icon_counts[military_unit->get_icon()];
		}
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	assert_throw(best_icon != nullptr);

	return best_icon;
}

QVariantList province_game_data::get_entering_armies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_entering_armies());
}

const std::vector<military_unit_category> &province_game_data::get_recruitable_military_unit_categories() const
{
	static const std::vector<military_unit_category> recruitable_military_unit_categories{
		military_unit_category::light_infantry,
		military_unit_category::regular_infantry,
		military_unit_category::heavy_infantry,
		military_unit_category::mace_infantry,
		military_unit_category::blade_infantry,
		military_unit_category::spear_infantry,
		military_unit_category::bowmen,
		military_unit_category::light_cavalry,
		military_unit_category::heavy_cavalry,
		military_unit_category::spear_cavalry,
		military_unit_category::light_artillery,
		military_unit_category::heavy_artillery
	};

	return recruitable_military_unit_categories;
}

QVariantList province_game_data::get_recruitable_military_unit_categories_qvariant_list() const
{
	return container::to_qvariant_list(this->get_recruitable_military_unit_categories());
}

const military_unit_type_map<int> &province_game_data::get_military_unit_recruitment_counts() const
{
	return this->military_unit_recruitment_counts;
}

QVariantList province_game_data::get_military_unit_recruitment_counts_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_unit_recruitment_counts());
}

void province_game_data::change_military_unit_recruitment_count(const military_unit_type *military_unit_type, const int change, const bool change_input_storage)
{
	if (change == 0) {
		return;
	}

	const int count = (this->military_unit_recruitment_counts[military_unit_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->military_unit_recruitment_counts.erase(military_unit_type);
	}

	if (change_input_storage) {
		assert_throw(this->get_owner() != nullptr);

		const int old_count = count - change;
		const commodity_map<int64_t> old_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, old_count);
		const commodity_map<int64_t> new_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, count);

		for (const auto &[commodity, cost] : new_commodity_costs) {
			assert_throw(commodity->is_storable());

			const int64_t cost_change = cost - old_commodity_costs.find(commodity)->second;

			this->get_owner()->get_economy()->change_stored_commodity(commodity, -cost_change);
		}
	}

	if (game::get()->is_running()) {
		emit military_unit_recruitment_counts_changed();
	}
}

bool province_game_data::can_increase_military_unit_recruitment(const military_unit_type *military_unit_type) const
{
	if (this->get_owner() == nullptr) {
		return false;
	}

	if (this->get_owner()->get_military()->get_best_military_unit_category_type(military_unit_type->get_category()) != military_unit_type) {
		return false;
	}

	const int old_count = this->get_military_unit_recruitment_count(military_unit_type);
	const int new_count = old_count + 1;
	const commodity_map<int64_t> old_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, old_count);
	const commodity_map<int64_t> new_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, new_count);

	for (const auto &[commodity, cost] : new_commodity_costs) {
		assert_throw(commodity->is_storable());

		const int64_t cost_change = cost - old_commodity_costs.find(commodity)->second;

		if (this->get_owner()->get_economy()->get_stored_commodity(commodity) < cost_change) {
			return false;
		}
	}

	return true;
}

void province_game_data::increase_military_unit_recruitment(const military_unit_type *military_unit_type)
{
	try {
		assert_throw(this->can_increase_military_unit_recruitment(military_unit_type));

		this->change_military_unit_recruitment_count(military_unit_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error increasing recruitment of the \"{}\" military unit type for country \"{}\" in province \"{}\".", military_unit_type->get_identifier(), this->get_owner()->get_identifier(), this->province->get_identifier())));
	}
}

bool province_game_data::can_decrease_military_unit_recruitment(const military_unit_type *military_unit_type) const
{
	if (this->get_military_unit_recruitment_count(military_unit_type) == 0) {
		return false;
	}

	return true;
}

void province_game_data::decrease_military_unit_recruitment(const military_unit_type *military_unit_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_military_unit_recruitment(military_unit_type));

		this->change_military_unit_recruitment_count(military_unit_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error decreasing recruitment of the \"{}\" military unit type for country \"{}\" in province \"{}\".", military_unit_type->get_identifier(), this->get_owner()->get_identifier(), this->province->get_identifier())));
	}
}

void province_game_data::clear_military_unit_recruitment_counts()
{
	this->military_unit_recruitment_counts.clear();
	emit military_unit_recruitment_counts_changed();
}

const std::vector<civilian_unit *> &province_game_data::get_civilian_units() const
{
	return this->civilian_units;
}

QVariantList province_game_data::get_civilian_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_civilian_units());
}

void province_game_data::add_civilian_unit(civilian_unit *civilian_unit)
{
	this->civilian_units.push_back(civilian_unit);

	emit civilian_units_changed();
}

void province_game_data::remove_civilian_unit(civilian_unit *civilian_unit)
{
	std::erase(this->civilian_units, civilian_unit);

	emit civilian_units_changed();
}

void province_game_data::calculate_site_commodity_outputs()
{
	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() && site->get_type() != site_type::resource && site->get_type() != site_type::celestial_body) {
			continue;
		}

		site->get_game_data()->calculate_commodity_outputs();
	}
}

void province_game_data::calculate_site_commodity_output(const commodity *commodity)
{
	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() && site->get_type() != site_type::resource && site->get_type() != site_type::celestial_body) {
			continue;
		}

		if (!site->get_game_data()->produces_commodity(commodity) && !site->get_game_data()->get_base_commodity_outputs().contains(commodity)) {
			continue;
		}

		site->get_game_data()->calculate_commodity_outputs();
	}
}

void province_game_data::change_local_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int &output = (this->local_commodity_outputs[commodity] += change);

	if (output == 0) {
		this->local_commodity_outputs.erase(commodity);
	}
}

void province_game_data::set_commodity_throughput_modifier(const commodity *commodity, const int value)
{
	if (value == this->get_commodity_throughput_modifier(commodity)) {
		return;
	}

	if (value == 0) {
		this->commodity_throughput_modifiers.erase(commodity);
	} else {
		this->commodity_throughput_modifiers[commodity] = value;
	}

	this->calculate_site_commodity_output(commodity);
}

void province_game_data::set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
{
	const int old_value = this->get_commodity_bonus_for_tile_threshold(commodity, threshold);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->commodity_bonuses_for_tile_thresholds[commodity].erase(threshold);

		if (this->commodity_bonuses_for_tile_thresholds[commodity].empty()) {
			this->commodity_bonuses_for_tile_thresholds.erase(commodity);
		}
	} else {
		this->commodity_bonuses_for_tile_thresholds[commodity][threshold] = value;
	}

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		tile *tile = map::get()->get_tile(tile_pos);
		if (!tile->produces_commodity(commodity)) {
			continue;
		}

		tile->calculate_commodity_outputs();
	}
}

bool province_game_data::can_produce_commodity(const commodity *commodity) const
{
	assert_throw(commodity != nullptr);

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const metternich::resource *tile_resource = tile->get_resource();
		const metternich::commodity *tile_resource_commodity = tile_resource->get_commodity();

		if (tile_resource_commodity == commodity) {
			return true;
		}
	}

	return false;
}

int64_t province_game_data::get_min_income() const
{
	const dice &taxation_dice = defines::get()->get_province_taxation_for_level(this->get_level());
	return std::max(0ll, taxation_dice.get_minimum_result() * defines::get()->get_domain_income_unit_value());
}

int64_t province_game_data::get_max_income() const
{
	const dice &taxation_dice = defines::get()->get_province_taxation_for_level(this->get_level());
	return taxation_dice.get_maximum_result() * defines::get()->get_domain_income_unit_value();
}

}
